/*
 * QuickTime splitter + decoder
 *
 * Copyright 2011 Aric Stewart for CodeWeavers
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

#include <QuickTime/Movies.h>
#include <QuickTime/QuickTimeComponents.h>

#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef _CDECL
#undef DPRINTF
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
#undef DPRINTF
#undef STDMETHODCALLTYPE

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "qtprivate.h"

WINE_DEFAULT_DEBUG_CHANNEL(qtsplitter);
extern CLSID CLSID_QTSplitter;

typedef struct QTOutPin {
    BaseOutputPin pin;

    AM_MEDIA_TYPE * pmt;
    OutputQueue * queue;
} QTOutPin;

typedef struct QTInPin {
    BasePin pin;
    GUID subType;

    IAsyncReader *pReader;
    IMemAllocator *pAlloc;
} QTInPin;

typedef struct QTSplitter {
    BaseFilter filter;

    QTInPin pInputPin;
    QTOutPin *pVideo_Pin;

    ALLOCATOR_PROPERTIES props;

    Movie pQTMovie;
    QTVisualContextRef vContext;

    DWORD outputSize;
    FILTER_STATE state;
} QTSplitter;

static const IPinVtbl QT_OutputPin_Vtbl;
static const IPinVtbl QT_InputPin_Vtbl;
static const IBaseFilterVtbl QT_Vtbl;

static HRESULT QT_AddPin(QTSplitter *This, const PIN_INFO *piOutput, const AM_MEDIA_TYPE *amt);
static HRESULT QT_RemoveOutputPins(QTSplitter *This);

/*
 * Base Filter
 */

static IPin* WINAPI QT_GetPin(BaseFilter *iface, int pos)
{
    QTSplitter *This = (QTSplitter *)iface;
    TRACE("Asking for pos %x\n", pos);

    if (pos > 2 || pos < 0)
        return NULL;
    switch (pos)
    {
        case 0:
            IPin_AddRef((IPin*)&This->pInputPin);
            return (IPin*)&This->pInputPin;
        case 1:
            if (This->pVideo_Pin)
                IPin_AddRef((IPin*)This->pVideo_Pin);
            return (IPin*)This->pVideo_Pin;
        case 2: /* TODO: Audio */
            return NULL;
        default:
            return NULL;
    }
}

static LONG WINAPI QT_GetPinCount(BaseFilter *iface)
{
    QTSplitter *This = (QTSplitter *)iface;
    int c = 1;
    if (This->pVideo_Pin) c++;
    return c;
}

static const BaseFilterFuncTable BaseFuncTable = {
    QT_GetPin,
    QT_GetPinCount
};

IUnknown * CALLBACK QTSplitter_create(IUnknown *punkout, HRESULT *phr)
{
    IUnknown *obj = NULL;
    PIN_INFO *piInput;
    QTSplitter *This;
    static const WCHAR wcsInputPinName[] = {'I','n','p','u','t',' ','P','i','n',0};

    EnterMovies();

    RegisterWineDataHandler();

    This = CoTaskMemAlloc(sizeof(*This));
    obj = (IUnknown*)This;
    if (!This)
    {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    BaseFilter_Init(&This->filter, &QT_Vtbl, &CLSID_QTSplitter, (DWORD_PTR)(__FILE__ ": QTSplitter.csFilter"), &BaseFuncTable);

    This->pVideo_Pin = NULL;
    This->state = State_Stopped;

    piInput = &This->pInputPin.pin.pinInfo;
    piInput->dir = PINDIR_INPUT;
    piInput->pFilter = (IBaseFilter *)This;
    lstrcpynW(piInput->achName, wcsInputPinName, sizeof(piInput->achName) / sizeof(piInput->achName[0]));
    This->pInputPin.pin.lpVtbl = &QT_InputPin_Vtbl;
    This->pInputPin.pin.refCount = 1;
    This->pInputPin.pin.pConnectedTo = NULL;
    This->pInputPin.pin.pCritSec = &This->filter.csFilter;
    ZeroMemory(&This->pInputPin.pin.mtCurrent, sizeof(AM_MEDIA_TYPE));
    *phr = S_OK;
    return obj;
}

static void QT_Destroy(QTSplitter *This)
{
    IPin *connected = NULL;
    ULONG pinref;

    TRACE("Destroying\n");

    /* Don't need to clean up output pins, disconnecting input pin will do that */
    IPin_ConnectedTo((IPin *)&This->pInputPin, &connected);
    if (connected)
    {
        IPin_Disconnect(connected);
        IPin_Release(connected);
    }
    pinref = IPin_Release((IPin *)&This->pInputPin);
    if (pinref)
    {
        ERR("pinref should be null, is %u, destroying anyway\n", pinref);
        assert((LONG)pinref > 0);

        while (pinref)
            pinref = IPin_Release((IPin *)&This->pInputPin);
    }

    if (This->pQTMovie)
        DisposeMovie(This->pQTMovie);
    if (This->vContext)
        QTVisualContextRelease(This->vContext);

    ExitMovies();
    CoTaskMemFree(This);
}

static HRESULT WINAPI QT_QueryInterface(IBaseFilter *iface, REFIID riid, LPVOID *ppv)
{
    QTSplitter *This = (QTSplitter *)iface;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI QT_Release(IBaseFilter *iface)
{
    QTSplitter *This = (QTSplitter *)iface;
    ULONG refCount = BaseFilterImpl_Release(iface);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
        QT_Destroy(This);

    return refCount;
}

static HRESULT WINAPI QT_Stop(IBaseFilter *iface)
{
    QTSplitter *This = (QTSplitter *)iface;

    TRACE("()\n");

    IAsyncReader_BeginFlush(This->pInputPin.pReader);
    IAsyncReader_EndFlush(This->pInputPin.pReader);

    return S_OK;
}

static HRESULT WINAPI QT_Pause(IBaseFilter *iface)
{
    HRESULT hr = S_OK;
    TRACE("()\n");

    return hr;
}

static DWORD WINAPI QTSplitter_thread_reader(LPVOID data)
{
    QTSplitter *This = (QTSplitter *)data;
    HRESULT hr = S_OK;
    TimeValue movie_time=0, next_time;
    CVPixelBufferRef pixelBuffer = NULL;
    OSStatus err;
    TimeRecord tr;

    This->state = State_Running;
    GetMovieTime(This->pQTMovie, &tr);

    do
    {
        LONGLONG tStart=0, tStop=0;
        float time;

        MoviesTask(This->pQTMovie,0);
        QTVisualContextTask(This->vContext);
        GetMovieNextInterestingTime(This->pQTMovie, nextTimeStep, 0, NULL, movie_time, 1, &next_time, NULL);

        if (next_time == -1)
        {
            TRACE("No next time\n");
            break;
        }

        tr.value = SInt64ToWide(next_time);
        SetMovieTime(This->pQTMovie, &tr);
        MoviesTask(This->pQTMovie,0);
        QTVisualContextTask(This->vContext);

        TRACE("In loop at time %ld\n",movie_time);
        TRACE("In Next time %ld\n",next_time);

        time = (float)movie_time / tr.scale;
        tStart = time * 10000000;
        time = (float)next_time / tr.scale;
        tStop = time * 10000000;

        /* Deliver Video */
        if (QTVisualContextIsNewImageAvailable(This->vContext,0))
        {
            err = QTVisualContextCopyImageForTime(This->vContext, NULL, NULL, &pixelBuffer);
            if (err == noErr)
            {
                int data_size=0;
                BYTE* ptr;
                IMediaSample *sample = NULL;

                hr = BaseOutputPinImpl_GetDeliveryBuffer((BaseOutputPin*)This->pVideo_Pin, &sample, NULL, NULL, 0);
                if (FAILED(hr))
                {
                    ERR("Video: Unable to get delivery buffer (%x)\n", hr);
                    goto video_error;
                }

                data_size = IMediaSample_GetSize(sample);
                if (data_size < This->outputSize)
                {
                    ERR("Sample size is too small %d < %d\n", data_size, This->outputSize)
    ;
                    hr = E_FAIL;
                    goto video_error;
                }

                hr = IMediaSample_GetPointer(sample, &ptr);
                if (FAILED(hr))
                {
                    ERR("Video: Unable to get pointer to buffer (%x)\n", hr);
                    goto video_error;
                }

                hr = AccessPixelBufferPixels( pixelBuffer, ptr);
                if (FAILED(hr))
                {
                    ERR("Failed to access Pixels\n");
                    goto video_error;
                }

                IMediaSample_SetActualDataLength(sample, This->outputSize);

                IMediaSample_SetMediaTime(sample, &tStart, &tStop);
                if (tStart)
                    IMediaSample_SetTime(sample, &tStart, &tStop);
                else
                    IMediaSample_SetTime(sample, NULL, NULL);

                hr = OutputQueue_Receive(This->pVideo_Pin->queue, sample);
                TRACE("Video Delivered (%x)\n",hr);

    video_error:
                if (sample)
                    IMediaSample_Release(sample);
                if (pixelBuffer)
                    CVPixelBufferRelease(pixelBuffer);
            }
        }

        movie_time = next_time;
    } while (hr == S_OK);

    This->state = State_Stopped;
    OutputQueue_EOS(This->pVideo_Pin->queue);

    return hr;
}

static HRESULT WINAPI QT_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    QTSplitter *This = (QTSplitter *)iface;
    HRESULT hr_any = VFW_E_NOT_CONNECTED;
    DWORD tid;

    TRACE("(%s)\n", wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->filter.csFilter);
    This->filter.rtStreamStart = tStart;

    if (This->pVideo_Pin)
        hr = BaseOutputPinImpl_Active((BaseOutputPin *)This->pVideo_Pin);
    if (SUCCEEDED(hr))
        hr_any = hr;

    hr = hr_any;
    LeaveCriticalSection(&This->filter.csFilter);

    CreateThread(NULL, 0, QTSplitter_thread_reader, This, 0, &tid);
    TRACE("Created thread 0x%08x\n",tid);

    return hr;
}

static HRESULT WINAPI QT_GetState(IBaseFilter *iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    QTSplitter *This = (QTSplitter *)iface;
    TRACE("(%d, %p)\n", dwMilliSecsTimeout, pState);

    *pState = This->state;

    return S_OK;
}

static HRESULT WINAPI QT_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("(%p)->(%s,%p) stub\n", iface, debugstr_w(Id), ppPin);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl QT_Vtbl = {
    QT_QueryInterface,
    BaseFilterImpl_AddRef,
    QT_Release,
    BaseFilterImpl_GetClassID,
    QT_Stop,
    QT_Pause,
    QT_Run,
    QT_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    QT_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

/*
 * Input Pin
 */
static HRESULT QT_RemoveOutputPins(QTSplitter *This)
{
    HRESULT hr;
    TRACE("(%p)\n", This);

    if (This->pVideo_Pin)
    {
        hr = BaseOutputPinImpl_BreakConnect(&This->pVideo_Pin->pin);
        TRACE("Disconnect: %08x\n", hr);
        IPin_Release((IPin*)This->pVideo_Pin);
        This->pVideo_Pin = NULL;
    }

    BaseFilterImpl_IncrementPinVersion((BaseFilter*)This);
    return S_OK;
}

static ULONG WINAPI QTInPin_Release(IPin *iface)
{
    QTInPin *This = (QTInPin*)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.refCount);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);
    if (!refCount)
    {
        FreeMediaType(&This->pin.mtCurrent);
        if (This->pAlloc)
            IMemAllocator_Release(This->pAlloc);
        This->pAlloc = NULL;
        This->pin.lpVtbl = NULL;
        return 0;
    }
    else
        return refCount;
}

static HRESULT QT_Process_Video_Track(QTSplitter* filter, Track trk)
{
    AM_MEDIA_TYPE amt;
    VIDEOINFOHEADER * pvi;
    PIN_INFO piOutput;
    HRESULT hr = S_OK;
    OSErr err;
    static const WCHAR szwVideoOut[] = {'V','i','d','e','o',0};
    CFMutableDictionaryRef  pixelBufferOptions = NULL;
    CFMutableDictionaryRef  visualContextOptions = NULL;
    CFNumberRef n = NULL;
    int t;
    DWORD outputWidth, outputHeight, outputDepth;
    Fixed trackWidth, trackHeight;

    ZeroMemory(&amt, sizeof(amt));
    amt.formattype = FORMAT_VideoInfo;
    amt.majortype = MEDIATYPE_Video;
    amt.subtype = MEDIASUBTYPE_RGB24;

    GetTrackDimensions(trk, &trackWidth, &trackHeight);

    outputDepth = 3;
    outputWidth = Fix2Long(trackWidth);
    outputHeight = Fix2Long(trackHeight);
    TRACE("Width %i  Height %i\n",outputWidth, outputHeight);

    amt.cbFormat = sizeof(VIDEOINFOHEADER);
    amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
    ZeroMemory(amt.pbFormat, amt.cbFormat);
    pvi = (VIDEOINFOHEADER *)amt.pbFormat;
    pvi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = outputWidth;
    pvi->bmiHeader.biHeight = -outputHeight;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biCompression = BI_RGB;

    filter->outputSize = outputWidth * outputHeight * outputDepth;
    amt.lSampleSize = 0;

    pixelBufferOptions = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    t = k32ARGBPixelFormat;
    n = CFNumberCreate(NULL, kCFNumberIntType, &t);
    CFDictionaryAddValue(pixelBufferOptions, kCVPixelBufferPixelFormatTypeKey, n);
    CFRelease(n);

    n = CFNumberCreate(NULL, kCFNumberIntType, &outputWidth);
    CFDictionaryAddValue(pixelBufferOptions, kCVPixelBufferWidthKey, n);
    CFRelease(n);

    n = CFNumberCreate(NULL, kCFNumberIntType, &outputHeight);
    CFDictionaryAddValue(pixelBufferOptions, kCVPixelBufferHeightKey, n);
    CFRelease(n);

    t = 16;
    n = CFNumberCreate(NULL, kCFNumberIntType, &t);
    CFDictionaryAddValue(pixelBufferOptions, kCVPixelBufferBytesPerRowAlignmentKey, n);
    CFRelease(n);

    visualContextOptions = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFDictionarySetValue(visualContextOptions, kQTVisualContextPixelBufferAttributesKey, pixelBufferOptions);

    err = QTPixelBufferContextCreate(NULL, visualContextOptions,&filter->vContext);
    CFRelease(pixelBufferOptions);
    CFRelease(visualContextOptions);
    if (err != noErr)
    {
        ERR("Failed to create Visual Context\n");
        return E_FAIL;
    }

    err = SetMovieVisualContext(filter->pQTMovie, filter->vContext);
    if (err != noErr)
    {
        ERR("Failed to set Visual Context\n");
        return E_FAIL;
    }

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)filter;
    lstrcpyW(piOutput.achName,szwVideoOut);

    hr = QT_AddPin(filter, &piOutput, &amt);
    if (FAILED(hr))
        ERR("Failed to add Video Track\n");
     else
        TRACE("Video Pin %p\n",filter->pVideo_Pin);

    return hr;
}

static HRESULT QT_Process_Movie(QTSplitter* filter)
{
    HRESULT hr = S_OK;
    OSErr err;
    WineDataRefRecord ptrDataRefRec;
    Handle dataRef = NULL;
    Track trk;
    short id = 0;

    TRACE("Trying movie connect\n");

    ptrDataRefRec.pReader = filter->pInputPin.pReader;
    ptrDataRefRec.streamSubtype = filter->pInputPin.subType;
    PtrToHand( &ptrDataRefRec, &dataRef, sizeof(WineDataRefRecord));

    err = NewMovieFromDataRef(&filter->pQTMovie, newMovieActive|newMovieDontInteractWithUser|newMovieDontAutoUpdateClock|newMovieDontAskUnresolvedDataRefs|newMovieIdleImportOK, &id, dataRef, 'WINE');

    if (err != noErr)
    {
        FIXME("Quicktime cannot handle media type(%i)\n",err);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    PrePrerollMovie(filter->pQTMovie, 0, fixed1, NULL, NULL);
    PrerollMovie(filter->pQTMovie, 0, fixed1);
    GoToBeginningOfMovie(filter->pQTMovie);
    SetMovieActive(filter->pQTMovie,TRUE);

    if (GetMovieLoadState(filter->pQTMovie) < kMovieLoadStateLoaded)
        MoviesTask(filter->pQTMovie,100);

    trk = GetMovieIndTrackType(filter->pQTMovie, 1, VisualMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly);
    TRACE("%p is a video track\n",trk);
    if (trk)
       hr = QT_Process_Video_Track(filter, trk);

    if (!SUCCEEDED(hr))
        return hr;

    return hr;
}

static HRESULT WINAPI QTInPin_ReceiveConnection(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = S_OK;
    ALLOCATOR_PROPERTIES props;
    QTInPin *This = (QTInPin*)iface;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pReceivePin, pmt);

    EnterCriticalSection(This->pin.pCritSec);
    This->pReader = NULL;

    if (This->pin.pConnectedTo)
        hr = VFW_E_ALREADY_CONNECTED;
    else if (IPin_QueryAccept(iface, pmt) != S_OK)
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    else
    {
        PIN_DIRECTION pindirReceive;
        IPin_QueryDirection(pReceivePin, &pindirReceive);
        if (pindirReceive != PINDIR_OUTPUT)
            hr = VFW_E_INVALID_DIRECTION;
    }

    if (!SUCCEEDED(hr))
    {
        LeaveCriticalSection(This->pin.pCritSec);
        return hr;
    }

    hr = IPin_QueryInterface(pReceivePin, &IID_IAsyncReader, (LPVOID *)&This->pReader);
    if (!SUCCEEDED(hr))
    {
        LeaveCriticalSection(This->pin.pCritSec);
        TRACE("Input source is not an AsyncReader\n");
        return hr;
    }

    LeaveCriticalSection(This->pin.pCritSec);
    EnterCriticalSection(&((QTSplitter *)This->pin.pinInfo.pFilter)->filter.csFilter);
    hr = QT_Process_Movie((QTSplitter *)This->pin.pinInfo.pFilter);
    if (!SUCCEEDED(hr))
    {
        LeaveCriticalSection(&((QTSplitter *)This->pin.pinInfo.pFilter)->filter.csFilter);
        TRACE("Unable to process movie\n");
        return hr;
    }

    This->pAlloc = NULL;
    props.cBuffers = 8;
    props.cbAlign = 1;
    props.cbBuffer = ((QTSplitter *)This->pin.pinInfo.pFilter)->outputSize + props.cbAlign;
    props.cbPrefix = 0;

    hr = IAsyncReader_RequestAllocator(This->pReader, NULL, &props, &This->pAlloc);
    if (SUCCEEDED(hr))
    {
        CopyMediaType(&This->pin.mtCurrent, pmt);
        This->pin.pConnectedTo = pReceivePin;
        IPin_AddRef(pReceivePin);
        hr = IMemAllocator_Commit(This->pAlloc);
    }
    else
    {
        QT_RemoveOutputPins((QTSplitter *)This->pin.pinInfo.pFilter);
        if (This->pReader)
            IAsyncReader_Release(This->pReader);
        This->pReader = NULL;
        if (This->pAlloc)
            IMemAllocator_Release(This->pAlloc);
        This->pAlloc = NULL;
    }
    TRACE("Size: %i\n", props.cbBuffer);
    LeaveCriticalSection(&((QTSplitter *)This->pin.pinInfo.pFilter)->filter.csFilter);

    return hr;
}

static HRESULT WINAPI QTInPin_Disconnect(IPin *iface)
{
    HRESULT hr;
    QTInPin *This = (QTInPin*)iface;
    FILTER_STATE state;
    TRACE("()\n");

    hr = IBaseFilter_GetState(This->pin.pinInfo.pFilter, INFINITE, &state);
    EnterCriticalSection(This->pin.pCritSec);
    if (This->pin.pConnectedTo)
    {
        QTSplitter *Parser = (QTSplitter *)This->pin.pinInfo.pFilter;

        if (SUCCEEDED(hr) && state == State_Stopped)
        {
            IMemAllocator_Decommit(This->pAlloc);
            IPin_Disconnect(This->pin.pConnectedTo);
            This->pin.pConnectedTo = NULL;
            hr = QT_RemoveOutputPins(Parser);
        }
        else
            hr = VFW_E_NOT_STOPPED;
    }
    else
        hr = S_FALSE;
    LeaveCriticalSection(This->pin.pCritSec);
    return hr;
}

static HRESULT WINAPI QTInPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    QTInPin *This = (QTInPin*)iface;

    TRACE("(%p)->(%p)\n", This, pmt);

    if (IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream))
    {
        This->subType = pmt->subtype;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI QTInPin_EndOfStream(IPin *iface)
{
    QTInPin *pin = (QTInPin*)iface;
    QTSplitter *This = (QTSplitter*)pin->pin.pinInfo.pFilter;

    FIXME("Propagate message on %p\n", This);
    return S_OK;
}

static HRESULT WINAPI QTInPin_BeginFlush(IPin *iface)
{
    QTInPin *pin = (QTInPin*)iface;
    QTSplitter *This = (QTSplitter*)pin->pin.pinInfo.pFilter;

    FIXME("Propagate message on %p\n", This);
    return S_OK;
}

static HRESULT WINAPI QTInPin_EndFlush(IPin *iface)
{
    QTInPin *pin = (QTInPin*)iface;
    QTSplitter *This = (QTSplitter*)pin->pin.pinInfo.pFilter;

    FIXME("Propagate message on %p\n", This);
    return S_OK;
}

static HRESULT WINAPI QTInPin_NewSegment(IPin *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    QTInPin *pin = (QTInPin*)iface;
    QTSplitter *This = (QTSplitter*)pin->pin.pinInfo.pFilter;

    BasePinImpl_NewSegment(iface, tStart, tStop, dRate);
    FIXME("Propagate message on %p\n", This);
    return S_OK;
}

static HRESULT WINAPI QTInPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    QTInPin *This = (QTInPin*)iface;

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static HRESULT WINAPI QTInPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return EnumMediaTypes_Construct(This, BasePinImpl_GetMediaType, BasePinImpl_GetMediaTypeVersion, ppEnum);
}

static const IPinVtbl QT_InputPin_Vtbl = {
    QTInPin_QueryInterface,
    BasePinImpl_AddRef,
    QTInPin_Release,
    BaseInputPinImpl_Connect,
    QTInPin_ReceiveConnection,
    QTInPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    QTInPin_QueryAccept,
    QTInPin_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    QTInPin_EndOfStream,
    QTInPin_BeginFlush,
    QTInPin_EndFlush,
    QTInPin_NewSegment
};

/*
 * Output Pin
 */

static HRESULT WINAPI QTOutPin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    QTOutPin *This = (QTOutPin *)iface;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IBaseFilter_QueryInterface(This->pin.pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }
    FIXME("No interface for %s!\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI QTOutPin_Release(IPin *iface)
{
    QTOutPin *This = (QTOutPin *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.pin.refCount);
    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        DeleteMediaType(This->pmt);
        FreeMediaType(&This->pin.pin.mtCurrent);
        OutputQueue_Destroy(This->queue);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI QTOutPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    QTOutPin *This = (QTOutPin *)iface;

    if (iPosition < 0)
        return E_INVALIDARG;
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, This->pmt);
    return S_OK;
}

static HRESULT WINAPI QTOutPin_DecideBufferSize(BaseOutputPin *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    /* Unused */
    return S_OK;
}

static HRESULT WINAPI QTOutPin_DecideAllocator(BaseOutputPin *iface, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    HRESULT hr;
    QTOutPin *This = (QTOutPin *)iface;
    QTSplitter *QTfilter = (QTSplitter*)This->pin.pin.pinInfo.pFilter;

    pAlloc = NULL;
    if (QTfilter->pInputPin.pAlloc)
        hr = IMemInputPin_NotifyAllocator(pPin, QTfilter->pInputPin.pAlloc, FALSE);
    else
        hr = VFW_E_NO_ALLOCATOR;

    return hr;
}

static HRESULT WINAPI QTOutPin_BreakConnect(BaseOutputPin *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    if (!This->pin.pConnectedTo || !This->pMemInputPin)
        hr = VFW_E_NOT_CONNECTED;
    else
    {
        hr = IPin_Disconnect(This->pin.pConnectedTo);
        IPin_Disconnect((IPin *)This);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static const IPinVtbl QT_OutputPin_Vtbl = {
    QTOutPin_QueryInterface,
    BasePinImpl_AddRef,
    QTOutPin_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static const BasePinFuncTable output_BaseFuncTable = {
    NULL,
    BaseOutputPinImpl_AttemptConnection,
    BasePinImpl_GetMediaTypeVersion,
    QTOutPin_GetMediaType
};

static const BaseOutputPinFuncTable output_BaseOutputFuncTable = {
    QTOutPin_DecideBufferSize,
    QTOutPin_DecideAllocator,
    QTOutPin_BreakConnect
};

static const OutputQueueFuncTable output_OutputQueueFuncTable = {
    OutputQueueImpl_ThreadProc
};

static HRESULT QT_AddPin(QTSplitter *This, const PIN_INFO *piOutput, const AM_MEDIA_TYPE *amt)
{
    HRESULT hr;
    IPin **target;

    target = (IPin**)&This->pVideo_Pin;

    if (*target != NULL)
    {
        ERR("We we already have a video pin\n");
        return E_FAIL;
    }

    hr = BaseOutputPin_Construct(&QT_OutputPin_Vtbl, sizeof(QTOutPin), piOutput, &output_BaseFuncTable, &output_BaseOutputFuncTable, &This->filter.csFilter, (IPin**)target);
    if (SUCCEEDED(hr))
    {
        QTOutPin *pin = (QTOutPin*)*target;
        pin->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        CopyMediaType(pin->pmt, amt);
        pin->pin.pin.pinInfo.pFilter = (LPVOID)This;

        BaseFilterImpl_IncrementPinVersion((BaseFilter*)This);

        hr = OutputQueue_Construct((BaseOutputPin*)pin, TRUE, TRUE, 5, FALSE, THREAD_PRIORITY_NORMAL, &output_OutputQueueFuncTable, &pin->queue);
    }
    else
        ERR("Failed with error %x\n", hr);
    return hr;
}
