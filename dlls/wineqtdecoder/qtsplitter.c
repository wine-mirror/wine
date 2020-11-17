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

#include "wine/heap.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "qtprivate.h"
#include "wineqtdecoder_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(qtsplitter);

typedef struct QTOutPin {
    struct strmbase_source pin;
    IQualityControl IQualityControl_iface;
    SourceSeeking seeking;

    AM_MEDIA_TYPE * pmt;
    OutputQueue * queue;
} QTOutPin;

typedef struct QTInPin {
    struct strmbase_sink pin;
    GUID subType;

    IAsyncReader *pReader;
    IMemAllocator *pAlloc;
} QTInPin;

typedef struct QTSplitter {
    struct strmbase_filter filter;

    QTInPin pInputPin;
    QTOutPin *pVideo_Pin;
    QTOutPin *pAudio_Pin;

    ALLOCATOR_PROPERTIES props;

    Movie pQTMovie;
    QTVisualContextRef vContext;

    MovieAudioExtractionRef aSession;
    HANDLE runEvent;

    DWORD outputSize;
    CRITICAL_SECTION csReceive;

    TimeValue movie_time;
    TimeValue movie_start;
    TimeScale movie_scale;

    HANDLE loaderThread;
    HANDLE splitterThread;
} QTSplitter;

static const IBaseFilterVtbl QT_Vtbl;
static const IMediaSeekingVtbl QT_Seeking_Vtbl;

static HRESULT QT_AddPin(QTSplitter *filter, const WCHAR *name, const AM_MEDIA_TYPE *mt, BOOL video);
static HRESULT QT_RemoveOutputPins(QTSplitter *This);

static HRESULT WINAPI QTSplitter_ChangeStart(IMediaSeeking *iface);
static HRESULT WINAPI QTSplitter_ChangeStop(IMediaSeeking *iface);
static HRESULT WINAPI QTSplitter_ChangeRate(IMediaSeeking *iface);

static inline QTOutPin *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, QTOutPin, seeking.IMediaSeeking_iface);
}

static inline QTSplitter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, QTSplitter, filter);
}

static inline QTSplitter *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, QTSplitter, filter.IBaseFilter_iface);
}

static inline QTInPin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, QTInPin, pin.pin.IPin_iface);
}

/*
 * Base Filter
 */

static struct strmbase_pin *qt_splitter_get_pin(struct strmbase_filter *base, unsigned int index)
{
    QTSplitter *filter = impl_from_strmbase_filter(base);

    if (index == 0)
        return &filter->pInputPin.pin.pin;
    else if (index == 1)
    {
        if (filter->pVideo_Pin)
            return &filter->pVideo_Pin->pin.pin;
        else if (filter->pAudio_Pin)
            return &filter->pAudio_Pin->pin.pin;
    }
    else if (index == 2 && filter->pVideo_Pin && filter->pAudio_Pin)
        return &filter->pAudio_Pin->pin.pin;

    return NULL;
}

static void qt_splitter_destroy(struct strmbase_filter *iface)
{
    QTSplitter *filter = impl_from_strmbase_filter(iface);

    EnterCriticalSection(&filter->csReceive);
    /* Don't need to clean up output pins, disconnecting input pin will do that */
    if (filter->pInputPin.pin.pin.peer)
        IPin_Disconnect(filter->pInputPin.pin.pin.peer);

    if (filter->pInputPin.pAlloc)
        IMemAllocator_Release(filter->pInputPin.pAlloc);
    filter->pInputPin.pAlloc = NULL;
    if (filter->pInputPin.pReader)
        IAsyncReader_Release(filter->pInputPin.pReader);
    filter->pInputPin.pReader = NULL;

    if (filter->pQTMovie)
    {
        DisposeMovie(filter->pQTMovie);
        filter->pQTMovie = NULL;
    }
    if (filter->vContext)
        QTVisualContextRelease(filter->vContext);
    if (filter->aSession)
        MovieAudioExtractionEnd(filter->aSession);

    ExitMoviesOnThread();
    LeaveCriticalSection(&filter->csReceive);

    if (filter->loaderThread)
    {
        WaitForSingleObject(filter->loaderThread, INFINITE);
        CloseHandle(filter->loaderThread);
    }
    if (filter->splitterThread)
    {
        SetEvent(filter->runEvent);
        WaitForSingleObject(filter->splitterThread, INFINITE);
        CloseHandle(filter->splitterThread);
    }

    CloseHandle(filter->runEvent);

    filter->csReceive.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->csReceive);
    strmbase_sink_cleanup(&filter->pInputPin.pin);
    strmbase_filter_cleanup(&filter->filter);

    CoTaskMemFree(filter);
}

static HRESULT qt_splitter_start_stream(struct strmbase_filter *iface, REFERENCE_TIME time)
{
    QTSplitter *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    EnterCriticalSection(&filter->csReceive);

    if (filter->pVideo_Pin && filter->pVideo_Pin->pin.pin.peer
            && FAILED(hr = IMemAllocator_Commit(filter->pVideo_Pin->pin.pAllocator)))
        ERR("Failed to commit video allocator, hr %#x.\n", hr);
    if (filter->pAudio_Pin && filter->pAudio_Pin->pin.pin.peer
            && FAILED(hr = IMemAllocator_Commit(filter->pAudio_Pin->pin.pAllocator)))
        ERR("Failed to commit audio allocator, hr %#x.\n", hr);
    SetEvent(filter->runEvent);

    LeaveCriticalSection(&filter->csReceive);

    return S_OK;
}

static HRESULT qt_splitter_cleanup_stream(struct strmbase_filter *iface)
{
    QTSplitter *filter = impl_from_strmbase_filter(iface);

    EnterCriticalSection(&filter->csReceive);
    IAsyncReader_BeginFlush(filter->pInputPin.pReader);
    IAsyncReader_EndFlush(filter->pInputPin.pReader);
    LeaveCriticalSection(&filter->csReceive);

    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = qt_splitter_get_pin,
    .filter_destroy = qt_splitter_destroy,
    .filter_start_stream = qt_splitter_start_stream,
    .filter_cleanup_stream = qt_splitter_cleanup_stream,
};

static HRESULT sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    QTInPin *pin = impl_from_IPin(&iface->IPin_iface);

    if (IsEqualGUID(&mt->majortype, &MEDIATYPE_Stream))
    {
        pin->subType = mt->subtype;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT QT_Process_Movie(QTSplitter *filter);

static HRESULT qt_splitter_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    QTSplitter *filter = impl_from_strmbase_filter(iface->pin.filter);
    ALLOCATOR_PROPERTIES props;
    IMemAllocator *allocator;
    HRESULT hr = S_OK;

    filter->pInputPin.pReader = NULL;

    if (FAILED(hr = IPin_QueryInterface(peer, &IID_IAsyncReader, (void **)&filter->pInputPin.pReader)))
        return hr;

    if (FAILED(hr = QT_Process_Movie(filter)))
    {
        IAsyncReader_Release(filter->pInputPin.pReader);
        filter->pInputPin.pReader = NULL;
        return hr;
    }

    filter->pInputPin.pAlloc = NULL;
    props.cBuffers = 8;
    props.cbAlign = 1;
    props.cbBuffer = filter->outputSize + props.cbAlign;
    props.cbPrefix = 0;

    /* Some applications depend on IAsyncReader::RequestAllocator() passing a
     * non-NULL preferred allocator. */
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC,
            &IID_IMemAllocator, (void **)&allocator);
    if (FAILED(hr))
        goto err;

    hr = IAsyncReader_RequestAllocator(filter->pInputPin.pReader, allocator, &props, &filter->pInputPin.pAlloc);
    IMemAllocator_Release(allocator);
    if (FAILED(hr))
    {
        WARN("Failed to get allocator, hr %#x.\n", hr);
        goto err;
    }

    if (FAILED(hr = IMemAllocator_Commit(filter->pInputPin.pAlloc)))
    {
        WARN("Failed to commit allocator, hr %#x.\n", hr);
        goto err;
    }

    return S_OK;
err:
    QT_RemoveOutputPins(filter);
    IAsyncReader_Release(filter->pInputPin.pReader);
    return hr;
}

static void qt_splitter_sink_disconnect(struct strmbase_sink *iface)
{
    QTSplitter *filter = impl_from_strmbase_filter(iface->pin.filter);

    IMemAllocator_Decommit(filter->pInputPin.pAlloc);
    QT_RemoveOutputPins(filter);
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .sink_connect = qt_splitter_sink_connect,
    .sink_disconnect = qt_splitter_sink_disconnect,
};

HRESULT qt_splitter_create(IUnknown *outer, IUnknown **out)
{
    QTSplitter *This;
    static const WCHAR wcsInputPinName[] = {'I','n','p','u','t',' ','P','i','n',0};

    EnterMoviesOnThread(0);

    RegisterWineDataHandler();

    if (!(This = CoTaskMemAlloc(sizeof(*This))))
        return E_OUTOFMEMORY;
    ZeroMemory(This,sizeof(*This));

    strmbase_filter_init(&This->filter, outer, &CLSID_QTSplitter, &filter_ops);
    strmbase_sink_init(&This->pInputPin.pin, &This->filter, wcsInputPinName, &sink_ops, NULL);

    InitializeCriticalSection(&This->csReceive);
    This->csReceive.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": QTSplitter.csReceive");

    This->pVideo_Pin = NULL;
    This->pAudio_Pin = NULL;
    This->aSession = NULL;
    This->runEvent = CreateEventW(NULL, 0, 0, NULL);

    TRACE("Created QT splitter %p.\n", This);
    *out = &This->filter.IUnknown_inner;
    return S_OK;
}

static OSErr QT_Create_Extract_Session(QTSplitter *filter)
{
    AudioStreamBasicDescription aDesc;
    OSErr err;
    WAVEFORMATEX* pvi;

    pvi = (WAVEFORMATEX*)filter->pAudio_Pin->pmt->pbFormat;

    err = MovieAudioExtractionBegin(filter->pQTMovie, 0, &filter->aSession);
    if (err != noErr)
    {
        ERR("Failed to begin Extraction session %i\n",err);
        return err;
    }

    err = MovieAudioExtractionGetProperty(filter->aSession,
            kQTPropertyClass_MovieAudioExtraction_Audio,
kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
            sizeof(AudioStreamBasicDescription), &aDesc, NULL);

    if (err != noErr)
    {
        MovieAudioExtractionEnd(filter->aSession);
        filter->aSession = NULL;
        ERR("Failed to get session description %i\n",err);
        return err;
    }

    aDesc.mFormatID = kAudioFormatLinearPCM;
    aDesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger +
        kAudioFormatFlagIsPacked;
    aDesc.mFramesPerPacket = 1;
    aDesc.mChannelsPerFrame = pvi->nChannels;
    aDesc.mBitsPerChannel = pvi->wBitsPerSample;
    aDesc.mSampleRate = pvi->nSamplesPerSec;
    aDesc.mBytesPerFrame = (aDesc.mBitsPerChannel * aDesc.mChannelsPerFrame) / 8;
    aDesc.mBytesPerPacket = aDesc.mBytesPerFrame * aDesc.mFramesPerPacket;

    err = MovieAudioExtractionSetProperty(filter->aSession,
        kQTPropertyClass_MovieAudioExtraction_Audio,
kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
        sizeof(AudioStreamBasicDescription), &aDesc);

    if (aDesc.mFormatID != kAudioFormatLinearPCM)
    {
        ERR("Not PCM Wave\n");
        err = -1;
    }
    if (aDesc.mFormatFlags != kLinearPCMFormatFlagIsSignedInteger +
        kAudioFormatFlagIsPacked)
    {
        ERR("Unhandled Flags\n");
        err = -1;
    }
    if (aDesc.mFramesPerPacket != 1)
    {
        ERR("Unhandled Frames per packet %li\n",aDesc.mFramesPerPacket);
        err = -1;
    }
    if (aDesc.mChannelsPerFrame != pvi->nChannels)
    {
        ERR("Unhandled channel count %li\n",aDesc.mChannelsPerFrame);
        err = -1;
    }
    if (aDesc.mBitsPerChannel != pvi->wBitsPerSample)
    {
        ERR("Unhandled bits per channel %li\n",aDesc.mBitsPerChannel);
        err = -1;
    }
    if (aDesc.mSampleRate != pvi->nSamplesPerSec)
    {
        ERR("Unhandled sample rate %f\n",aDesc.mSampleRate);
        err = -1;
    }

    if (err != noErr)
    {
        ERR("Failed to create Extraction Session\n");
        MovieAudioExtractionEnd(filter->aSession);
        filter->aSession = NULL;
    }

    return err;
}

static DWORD WINAPI QTSplitter_loading_thread(LPVOID data)
{
    QTSplitter *This = (QTSplitter *)data;

    if (This->pAudio_Pin)
    {
        /* according to QA1469 a movie has to be fully loaded before we
           can reliably start the Extraction session.

           If loaded earlier, then we only get an extraction session for
           the part of the movie that is loaded at that time.

            We are trying to load as much of the movie as we can before we
            start extracting.  However we can recreate the extraction session
            again when we run out of loaded extraction frames. But we want
            to try to minimize that.
         */

        EnterCriticalSection(&This->csReceive);
        while(This->pQTMovie && GetMovieLoadState(This->pQTMovie) < kMovieLoadStateComplete)
        {
            MoviesTask(This->pQTMovie, 100);
            LeaveCriticalSection(&This->csReceive);
            Sleep(0);
            EnterCriticalSection(&This->csReceive);
        }
        LeaveCriticalSection(&This->csReceive);
    }
    return 0;
}

static DWORD WINAPI QTSplitter_thread(LPVOID data)
{
    QTSplitter *This = (QTSplitter *)data;
    HRESULT hr = S_OK;
    TimeValue next_time;
    CVPixelBufferRef pixelBuffer = NULL;
    OSStatus err;
    TimeRecord tr;

    WaitForSingleObject(This->runEvent, -1);

    EnterCriticalSection(&This->csReceive);
    if (!This->pQTMovie)
    {
        LeaveCriticalSection(&This->csReceive);
        return 0;
    }

    /* Prime the pump:  Needed for MPEG streams */
    GetMovieNextInterestingTime(This->pQTMovie, nextTimeEdgeOK | nextTimeStep, 0, NULL, This->movie_time, 1, &next_time, NULL);

    GetMovieTime(This->pQTMovie, &tr);

    if (This->pAudio_Pin)
        QT_Create_Extract_Session(This);

    LeaveCriticalSection(&This->csReceive);

    do
    {
        LONGLONG tStart=0, tStop=0;
        LONGLONG mStart=0, mStop=0;
        float time;

        EnterCriticalSection(&This->csReceive);
        if (!This->pQTMovie)
        {
            LeaveCriticalSection(&This->csReceive);
            return 0;
        }

        GetMovieNextInterestingTime(This->pQTMovie, nextTimeStep, 0, NULL, This->movie_time, 1, &next_time, NULL);

        if (next_time == -1)
        {
            TRACE("No next time\n");
            LeaveCriticalSection(&This->csReceive);
            break;
        }

        tr.value = SInt64ToWide(next_time);
        SetMovieTime(This->pQTMovie, &tr);
        MoviesTask(This->pQTMovie,0);
        QTVisualContextTask(This->vContext);

        TRACE("In loop at time %ld\n",This->movie_time);
        TRACE("In Next time %ld\n",next_time);

        mStart = This->movie_time;
        mStop = next_time;

        time = (float)(This->movie_time - This->movie_start) / This->movie_scale;
        tStart = time * 10000000;
        time = (float)(next_time - This->movie_start) / This->movie_scale;
        tStop = time * 10000000;

        /* Deliver Audio */
        if (This->pAudio_Pin && This->pAudio_Pin->pin.pin.peer && This->aSession)
        {
            int data_size=0;
            BYTE* ptr;
            IMediaSample *sample = NULL;
            AudioBufferList aData;
            UInt32 flags;
            UInt32 frames;
            WAVEFORMATEX* pvi;
            float duration;

            pvi = (WAVEFORMATEX*)This->pAudio_Pin->pmt->pbFormat;

            hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->pAudio_Pin->pin, &sample, NULL, NULL, 0);

            if (FAILED(hr))
            {
                ERR("Audio: Unable to get delivery buffer (%x)\n", hr);
                goto audio_error;
            }

            hr = IMediaSample_GetPointer(sample, &ptr);
            if (FAILED(hr))
            {
                ERR("Audio: Unable to get pointer to buffer (%x)\n", hr);
                goto audio_error;
            }

            duration = (float)next_time / This->movie_scale;
            time = (float)This->movie_time / This->movie_scale;
            duration -= time;
            frames = pvi->nSamplesPerSec * duration;
            TRACE("Need audio for %f seconds (%li frames)\n",duration,frames);

            data_size = IMediaSample_GetSize(sample);
            if (data_size < frames * pvi->nBlockAlign)
                FIXME("Audio buffer is too small\n");

            aData.mNumberBuffers = 1;
            aData.mBuffers[0].mNumberChannels = pvi->nChannels;
            aData.mBuffers[0].mDataByteSize = data_size;
            aData.mBuffers[0].mData = ptr;

            err = MovieAudioExtractionFillBuffer(This->aSession, &frames, &aData, &flags);
            if (frames == 0)
            {
                TimeRecord etr;

                /* Ran out of frames, Restart the extraction session */
                TRACE("Restarting extraction session\n");
                MovieAudioExtractionEnd(This->aSession);
                This->aSession = NULL;
                QT_Create_Extract_Session(This);

                etr = tr;
                etr.value = SInt64ToWide(This->movie_time);
                MovieAudioExtractionSetProperty(This->aSession,
                    kQTPropertyClass_MovieAudioExtraction_Movie,
                    kQTMovieAudioExtractionMoviePropertyID_CurrentTime,
                    sizeof(TimeRecord), &etr );

                frames = pvi->nSamplesPerSec * duration;
                aData.mNumberBuffers = 1;
                aData.mBuffers[0].mNumberChannels = pvi->nChannels;
                aData.mBuffers[0].mDataByteSize = data_size;
                aData.mBuffers[0].mData = ptr;

                MovieAudioExtractionFillBuffer(This->aSession, &frames, &aData, &flags);
            }

            TRACE("Got %i frames\n",(int)frames);

            IMediaSample_SetActualDataLength(sample, frames * pvi->nBlockAlign);

            IMediaSample_SetMediaTime(sample, &mStart, &mStop);
            IMediaSample_SetTime(sample, &tStart, &tStop);

            hr = OutputQueue_Receive(This->pAudio_Pin->queue, sample);
            TRACE("Audio Delivered (%x)\n",hr);

audio_error:
            if (sample)
                IMediaSample_Release(sample);
        }
        else
            TRACE("Audio Pin not connected or no Audio\n");

        /* Deliver Video */
        if (This->pVideo_Pin && QTVisualContextIsNewImageAvailable(This->vContext,0))
        {
            err = QTVisualContextCopyImageForTime(This->vContext, NULL, NULL, &pixelBuffer);
            if (err == noErr)
            {
                int data_size=0;
                BYTE* ptr;
                IMediaSample *sample = NULL;

                hr = BaseOutputPinImpl_GetDeliveryBuffer(&This->pVideo_Pin->pin, &sample, NULL, NULL, 0);
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

                IMediaSample_SetMediaTime(sample, &mStart, &mStop);
                IMediaSample_SetTime(sample, &tStart, &tStop);

                hr = OutputQueue_Receive(This->pVideo_Pin->queue, sample);
                TRACE("Video Delivered (%x)\n",hr);

    video_error:
                if (sample)
                    IMediaSample_Release(sample);
                if (pixelBuffer)
                    CVPixelBufferRelease(pixelBuffer);
            }
        }
        else
            TRACE("No video to deliver\n");

        This->movie_time = next_time;
        LeaveCriticalSection(&This->csReceive);
    } while (hr == S_OK);

    if (This->pAudio_Pin)
        OutputQueue_EOS(This->pAudio_Pin->queue);
    if (This->pVideo_Pin)
        OutputQueue_EOS(This->pVideo_Pin->queue);

    return hr;
}

static void free_source_pin(QTOutPin *pin)
{
    if (pin->pin.pin.peer)
    {
        if (SUCCEEDED(IMemAllocator_Decommit(pin->pin.pAllocator)))
            IPin_Disconnect(pin->pin.pin.peer);
        IPin_Disconnect(&pin->pin.pin.IPin_iface);
    }

    DeleteMediaType(pin->pmt);
    strmbase_seeking_cleanup(&pin->seeking);
    strmbase_source_cleanup(&pin->pin);
    heap_free(pin);
}

/*
 * Input Pin
 */
static HRESULT QT_RemoveOutputPins(QTSplitter *This)
{
    if (This->pVideo_Pin)
    {
        OutputQueue_Destroy(This->pVideo_Pin->queue);
        free_source_pin(This->pVideo_Pin);
        This->pVideo_Pin = NULL;
    }
    if (This->pAudio_Pin)
    {
        OutputQueue_Destroy(This->pAudio_Pin->queue);
        free_source_pin(This->pAudio_Pin);
        This->pAudio_Pin = NULL;
    }

    BaseFilterImpl_IncrementPinVersion(&This->filter);
    return S_OK;
}

static HRESULT QT_Process_Video_Track(QTSplitter* filter, Track trk)
{
    AM_MEDIA_TYPE amt;
    VIDEOINFOHEADER * pvi;
    HRESULT hr = S_OK;
    OSErr err;
    static const WCHAR szwVideoOut[] = {'V','i','d','e','o',0};
    CFMutableDictionaryRef  pixelBufferOptions = NULL;
    CFMutableDictionaryRef  visualContextOptions = NULL;
    CFNumberRef n = NULL;
    int t;
    DWORD outputWidth, outputHeight, outputDepth;
    Fixed trackWidth, trackHeight;
    Media videoMedia;
    long sampleCount;
    TimeValue64 duration;
    TimeScale timeScale;

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
    pvi->bmiHeader.biHeight = outputHeight;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biSizeImage = outputWidth * outputHeight * outputDepth;

    filter->outputSize = pvi->bmiHeader.biSizeImage;
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

    videoMedia = GetTrackMedia(trk);
    sampleCount = GetMediaSampleCount(videoMedia);
    timeScale = GetMediaTimeScale(videoMedia);
    duration = GetMediaDisplayDuration(videoMedia);
    pvi->AvgTimePerFrame = (100000.0 * sampleCount * timeScale) / duration;

    hr = QT_AddPin(filter, szwVideoOut, &amt, TRUE);
    if (FAILED(hr))
        ERR("Failed to add Video Track\n");
     else
        TRACE("Video Pin %p\n",filter->pVideo_Pin);

    return hr;
}

static HRESULT QT_Process_Audio_Track(QTSplitter* filter, Track trk)
{
    AM_MEDIA_TYPE amt;
    WAVEFORMATEX* pvi;
    HRESULT hr = S_OK;
    static const WCHAR szwAudioOut[] = {'A','u','d','i','o',0};
    Media audioMedia;

    SoundDescriptionHandle  aDesc = (SoundDescriptionHandle) NewHandle(sizeof(SoundDescription));

    audioMedia = GetTrackMedia(trk);
    GetMediaSampleDescription(audioMedia, 1, (SampleDescriptionHandle)aDesc);

    ZeroMemory(&amt, sizeof(amt));
    amt.formattype = FORMAT_WaveFormatEx;
    amt.majortype = MEDIATYPE_Audio;
    amt.subtype = MEDIASUBTYPE_PCM;
    amt.bTemporalCompression = 0;

    amt.cbFormat = sizeof(WAVEFORMATEX);
    amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
    ZeroMemory(amt.pbFormat, amt.cbFormat);
    pvi = (WAVEFORMATEX*)amt.pbFormat;

    pvi->cbSize = sizeof(WAVEFORMATEX);
    pvi->wFormatTag = WAVE_FORMAT_PCM;
    pvi->nChannels = ((SoundDescription)**aDesc).numChannels;
    if (pvi->nChannels < 1 || pvi->nChannels > 2)
        pvi->nChannels = 2;
    pvi->nSamplesPerSec = (((SoundDescription)**aDesc).sampleRate/65536);
    if (pvi->nSamplesPerSec < 8000 || pvi->nChannels > 48000)
        pvi->nSamplesPerSec = 44100;
    pvi->wBitsPerSample = ((SoundDescription)**aDesc).sampleSize;
    if (pvi->wBitsPerSample < 8 || pvi->wBitsPerSample > 32)
        pvi->wBitsPerSample = 16;
    pvi->nBlockAlign = (pvi->nChannels * pvi->wBitsPerSample) / 8;
    pvi->nAvgBytesPerSec = pvi->nSamplesPerSec * pvi->nBlockAlign;

    DisposeHandle((Handle)aDesc);

    hr = QT_AddPin(filter, szwAudioOut, &amt, FALSE);
    if (FAILED(hr))
        ERR("Failed to add Audio Track\n");
    else
        TRACE("Audio Pin %p\n",filter->pAudio_Pin);
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
    DWORD tid;
    LONGLONG time, duration;

    TRACE("Trying movie connect\n");

    ptrDataRefRec.pReader = filter->pInputPin.pReader;
    ptrDataRefRec.streamSubtype = filter->pInputPin.subType;
    PtrToHand( &ptrDataRefRec, &dataRef, sizeof(WineDataRefRecord));

    err = NewMovieFromDataRef(&filter->pQTMovie, newMovieActive|newMovieDontInteractWithUser|newMovieDontAutoUpdateClock|newMovieDontAskUnresolvedDataRefs|newMovieAsyncOK, &id, dataRef, 'WINE');

    DisposeHandle(dataRef);

    if (err != noErr)
    {
        FIXME("QuickTime cannot handle media type(%i)\n",err);
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

    if (FAILED(hr))
        return hr;

    trk = GetMovieIndTrackType(filter->pQTMovie, 1, AudioMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly);
    TRACE("%p is an audio track\n",trk);
    if (trk)
        hr = QT_Process_Audio_Track(filter, trk);

    time = GetMovieDuration(filter->pQTMovie);
    filter->movie_scale = GetMovieTimeScale(filter->pQTMovie);
    duration = ((double)time / filter->movie_scale) * 10000000;
    TRACE("Movie duration is %s.\n", wine_dbgstr_longlong(duration));
    if (filter->pVideo_Pin)
        filter->pVideo_Pin->seeking.llStop = filter->pVideo_Pin->seeking.llDuration = duration;
    if (filter->pAudio_Pin)
        filter->pAudio_Pin->seeking.llStop = filter->pAudio_Pin->seeking.llDuration = duration;

    filter->loaderThread = CreateThread(NULL, 0, QTSplitter_loading_thread, filter, 0, &tid);
    if (filter->loaderThread)
        TRACE("Created loading thread 0x%08x\n", tid);
    filter->splitterThread = CreateThread(NULL, 0, QTSplitter_thread, filter, 0, &tid);
    if (filter->splitterThread)
        TRACE("Created processing thread 0x%08x\n", tid);
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

static inline QTOutPin *impl_source_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, QTOutPin, pin.pin);
}

static inline QTOutPin *impl_QTOutPin_from_BaseOutputPin(struct strmbase_source *iface)
{
    return CONTAINING_RECORD(iface, QTOutPin, pin);
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    QTOutPin *pin = impl_source_from_strmbase_pin(&iface->IPin_iface);

    if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &pin->seeking.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &pin->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT source_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *amt)
{
    FIXME("(%p) stub\n", base);
    return S_OK;
}

static HRESULT source_get_media_type(struct strmbase_pin *iface, unsigned int iPosition, AM_MEDIA_TYPE *pmt)
{
    QTOutPin *This = impl_source_from_strmbase_pin(iface);

    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, This->pmt);
    return S_OK;
}

static HRESULT WINAPI QTOutPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    /* Unused */
    return S_OK;
}

static HRESULT WINAPI QTOutPin_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    HRESULT hr;
    QTOutPin *This = impl_QTOutPin_from_BaseOutputPin(iface);
    QTSplitter *QTfilter = impl_from_strmbase_filter(This->pin.pin.filter);

    *pAlloc = NULL;
    if (QTfilter->pInputPin.pAlloc)
    {
        hr = IMemInputPin_NotifyAllocator(pPin, QTfilter->pInputPin.pAlloc, FALSE);
        if (SUCCEEDED(hr))
        {
            *pAlloc = QTfilter->pInputPin.pAlloc;
            IMemAllocator_AddRef(*pAlloc);
        }
    }
    else
        hr = VFW_E_NO_ALLOCATOR;

    return hr;
}

static inline QTOutPin *impl_from_IQualityControl( IQualityControl *iface )
{
    return CONTAINING_RECORD(iface, QTOutPin, IQualityControl_iface);
}

HRESULT WINAPI QT_QualityControl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv)
{
    QTOutPin *This = impl_from_IQualityControl(iface);
    return IPin_QueryInterface(&This->pin.pin.IPin_iface, riid, ppv);
}

ULONG WINAPI QT_QualityControl_AddRef(IQualityControl *iface)
{
    QTOutPin *This = impl_from_IQualityControl(iface);
    return IPin_AddRef(&This->pin.pin.IPin_iface);
}

ULONG WINAPI QT_QualityControl_Release(IQualityControl *iface)
{
    QTOutPin *This = impl_from_IQualityControl(iface);
    return IPin_Release(&This->pin.pin.IPin_iface);
}

static HRESULT WINAPI QT_QualityControl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm)
{
    REFERENCE_TIME late = qm.Late;
    if (qm.Late < 0 && -qm.Late > qm.TimeStamp)
        late = -qm.TimeStamp;
    /* TODO: Do Something */
    return S_OK;
}

HRESULT WINAPI QT_QualityControl_SetSink(IQualityControl *iface, IQualityControl *tonotify)
{
    /* Do nothing */
    return S_OK;
}

static const IQualityControlVtbl QTOutPin_QualityControl_Vtbl = {
    QT_QualityControl_QueryInterface,
    QT_QualityControl_AddRef,
    QT_QualityControl_Release,
    QT_QualityControl_Notify,
    QT_QualityControl_SetSink
};

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = QTOutPin_DecideBufferSize,
    .pfnDecideAllocator = QTOutPin_DecideAllocator,
};

static const OutputQueueFuncTable output_OutputQueueFuncTable = {
    OutputQueueImpl_ThreadProc
};

static HRESULT QT_AddPin(QTSplitter *filter, const WCHAR *name,
        const AM_MEDIA_TYPE *mt, BOOL video)
{
    QTOutPin *pin;

    if (!(pin = heap_alloc_zero(sizeof(*pin))))
        return E_OUTOFMEMORY;

    if (video)
        filter->pVideo_Pin = pin;
    else
        filter->pAudio_Pin = pin;

    strmbase_source_init(&pin->pin, &filter->filter, name, &source_ops);
    pin->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    CopyMediaType(pin->pmt, mt);
    pin->IQualityControl_iface.lpVtbl = &QTOutPin_QualityControl_Vtbl;
    strmbase_seeking_init(&pin->seeking, &QT_Seeking_Vtbl,
            QTSplitter_ChangeStop, QTSplitter_ChangeStart, QTSplitter_ChangeRate);
    BaseFilterImpl_IncrementPinVersion(&filter->filter);

    return OutputQueue_Construct(&pin->pin, TRUE, TRUE, 5, FALSE,
            THREAD_PRIORITY_NORMAL, &output_OutputQueueFuncTable, &pin->queue);
}

static HRESULT WINAPI QTSplitter_ChangeStart(IMediaSeeking *iface)
{
    QTOutPin *pin = impl_from_IMediaSeeking(iface);
    QTSplitter *filter = impl_from_strmbase_filter(pin->pin.pin.filter);
    TRACE("(%p)\n", iface);
    EnterCriticalSection(&filter->csReceive);
    filter->movie_start = filter->movie_time = (pin->seeking.llCurrent * filter->movie_scale)/10000000;
    LeaveCriticalSection(&filter->csReceive);
    return S_OK;
}

static HRESULT WINAPI QTSplitter_ChangeStop(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented stop position change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI QTSplitter_ChangeRate(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented rate change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI QT_Seeking_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    QTOutPin *pin = impl_from_IMediaSeeking(iface);
    return IPin_QueryInterface(&pin->pin.pin.IPin_iface, iid, out);
}

static ULONG WINAPI QT_Seeking_AddRef(IMediaSeeking * iface)
{
    QTOutPin *pin = impl_from_IMediaSeeking(iface);
    return IPin_AddRef(&pin->pin.pin.IPin_iface);
}

static ULONG WINAPI QT_Seeking_Release(IMediaSeeking * iface)
{
    QTOutPin *pin = impl_from_IMediaSeeking(iface);
    return IPin_Release(&pin->pin.pin.IPin_iface);
}

static const IMediaSeekingVtbl QT_Seeking_Vtbl =
{
    QT_Seeking_QueryInterface,
    QT_Seeking_AddRef,
    QT_Seeking_Release,
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
    SourceSeekingImpl_SetPositions,
    SourceSeekingImpl_GetPositions,
    SourceSeekingImpl_GetAvailable,
    SourceSeekingImpl_SetRate,
    SourceSeekingImpl_GetRate,
    SourceSeekingImpl_GetPreroll
};
