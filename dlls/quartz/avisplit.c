/*
 * AVI Splitter Filter
 *
 * Copyright 2003 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* FIXME:
 * - we don't do anything with indices yet (we could use them when seeking)
 * - we don't support multiple RIFF sections (i.e. large AVI files > 2Gb)
 */

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "aviriff.h"
#include "mmreg.h"
#include "vfwmsgs.h"
#include "amvideo.h"

#include "fourcc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include <math.h>
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const struct IBaseFilterVtbl AVISplitter_Vtbl;
static const struct IMediaSeekingVtbl AVISplitter_Seeking_Vtbl;
static const struct IPinVtbl AVISplitter_OutputPin_Vtbl;
static const struct IPinVtbl AVISplitter_InputPin_Vtbl;

static HRESULT AVISplitter_Sample(LPVOID iface, IMediaSample * pSample);
static HRESULT AVISplitter_OutputPin_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt);
static HRESULT AVISplitter_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt);
static HRESULT AVISplitter_InputPin_PreConnect(IPin * iface, IPin * pConnectPin);
static HRESULT AVISplitter_ChangeStart(LPVOID iface);
static HRESULT AVISplitter_ChangeStop(LPVOID iface);
static HRESULT AVISplitter_ChangeRate(LPVOID iface);

static HRESULT AVISplitter_InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

typedef struct AVISplitter
{
    const IBaseFilterVtbl * lpVtbl;

    ULONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    PullPin * pInputPin;
    ULONG cStreams;
    IPin ** ppPins;
    IMediaSample * pCurrentSample;
    RIFFCHUNK CurrentChunk;
    LONGLONG CurrentChunkOffset; /* in media time */
    LONGLONG EndOfFile;
    AVIMAINHEADER AviHeader;
} AVISplitter;

typedef struct AVISplitter_OutputPin
{
    OutputPin pin;

    AM_MEDIA_TYPE * pmt;
    float fSamplesPerSec;
    DWORD dwSamplesProcessed;
    DWORD dwSampleSize;
    DWORD dwLength;
    MediaSeekingImpl mediaSeeking;
} AVISplitter_OutputPin;


#define _IMediaSeeking_Offset ((int)(&(((AVISplitter_OutputPin*)0)->mediaSeeking)))
#define ICOM_THIS_From_IMediaSeeking(impl, iface) impl* This = (impl*)(((char*)iface)-_IMediaSeeking_Offset);

HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    AVISplitter * pAviSplit;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    pAviSplit = CoTaskMemAlloc(sizeof(AVISplitter));

    pAviSplit->lpVtbl = &AVISplitter_Vtbl;
    pAviSplit->refCount = 1;
    InitializeCriticalSection(&pAviSplit->csFilter);
    pAviSplit->state = State_Stopped;
    pAviSplit->pClock = NULL;
    pAviSplit->pCurrentSample = NULL;
    ZeroMemory(&pAviSplit->filterInfo, sizeof(FILTER_INFO));

    pAviSplit->cStreams = 0;
    pAviSplit->ppPins = CoTaskMemAlloc(1 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pAviSplit;
    strncpyW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = AVISplitter_InputPin_Construct(&piInput, AVISplitter_Sample, (LPVOID)pAviSplit, AVISplitter_QueryAccept, &pAviSplit->csFilter, (IPin **)&pAviSplit->pInputPin);

    if (SUCCEEDED(hr))
    {
        pAviSplit->ppPins[0] = (IPin *)pAviSplit->pInputPin;
        pAviSplit->pInputPin->fnPreConnect = AVISplitter_InputPin_PreConnect;
        *ppv = (LPVOID)pAviSplit;
    }
    else
    {
        CoTaskMemFree(pAviSplit->ppPins);
        DeleteCriticalSection(&pAviSplit->csFilter);
        CoTaskMemFree(pAviSplit);
    }

    return hr;
}

static HRESULT AVISplitter_OutputPin_Init(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES * props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, const AM_MEDIA_TYPE * pmt, float fSamplesPerSec, LPCRITICAL_SECTION pCritSec, AVISplitter_OutputPin * pPinImpl)
{
    pPinImpl->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    CopyMediaType(pPinImpl->pmt, pmt);
    pPinImpl->dwSamplesProcessed = 0;
    pPinImpl->dwSampleSize = 0;
    pPinImpl->fSamplesPerSec = fSamplesPerSec;

    MediaSeekingImpl_Init((LPVOID)pPinInfo->pFilter, AVISplitter_ChangeStop, AVISplitter_ChangeStart, AVISplitter_ChangeRate, &pPinImpl->mediaSeeking);
    pPinImpl->mediaSeeking.lpVtbl = &AVISplitter_Seeking_Vtbl;

    return OutputPin_Init(pPinInfo, props, pUserData, pQueryAccept, pCritSec, &pPinImpl->pin);
}

static HRESULT AVISplitter_OutputPin_Construct(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES * props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, const AM_MEDIA_TYPE * pmt, float fSamplesPerSec, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    AVISplitter_OutputPin * pPinImpl;

    *ppPin = NULL;

    assert(pPinInfo->dir == PINDIR_OUTPUT);

    pPinImpl = CoTaskMemAlloc(sizeof(AVISplitter_OutputPin));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(AVISplitter_OutputPin_Init(pPinInfo, props, pUserData, pQueryAccept, pmt, fSamplesPerSec, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.pin.lpVtbl = &AVISplitter_OutputPin_Vtbl;
        
        *ppPin = (IPin *)pPinImpl;
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI AVISplitter_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS(AVISplitter, iface);
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI AVISplitter_AddRef(IBaseFilter * iface)
{
    ICOM_THIS(AVISplitter, iface);
    TRACE("()\n");
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI AVISplitter_Release(IBaseFilter * iface)
{
    ICOM_THIS(AVISplitter, iface);
    TRACE("()\n");
    if (!InterlockedDecrement(&This->refCount))
    {
        ULONG i;

        DeleteCriticalSection(&This->csFilter);
        IReferenceClock_Release(This->pClock);
        
        for (i = 0; i < This->cStreams + 1; i++)
            IPin_Release(This->ppPins[i]);
        
        HeapFree(GetProcessHeap(), 0, This->ppPins);
        This->lpVtbl = NULL;
        
        TRACE("Destroying AVI splitter\n");
        CoTaskMemFree(This);
        
        return 0;
    }
    else
        return This->refCount;
}

/** IPersist methods **/

static HRESULT WINAPI AVISplitter_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    TRACE("(%p)\n", pClsid);

    *pClsid = CLSID_AviSplitter;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI AVISplitter_Stop(IBaseFilter * iface)
{
    HRESULT hr;
    ICOM_THIS(AVISplitter, iface);

    TRACE("()\n");

    EnterCriticalSection(&This->csFilter);
    {
        hr = PullPin_StopProcessing(This->pInputPin);

        This->state = State_Stopped;
    }
    LeaveCriticalSection(&This->csFilter);
    
    return hr;
}

static HRESULT WINAPI AVISplitter_Pause(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    BOOL bInit;
    ICOM_THIS(AVISplitter, iface);
    
    TRACE("()\n");

    EnterCriticalSection(&This->csFilter);
    {
        bInit = (This->state == State_Stopped);
        This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    if (bInit)
    {
        unsigned int i;

        hr = PullPin_Seek(This->pInputPin, This->CurrentChunkOffset, This->EndOfFile);

        if (SUCCEEDED(hr))
            hr = PullPin_InitProcessing(This->pInputPin);

        if (SUCCEEDED(hr))
        {
            for (i = 1; i < This->cStreams + 1; i++)
            {
                OutputPin_DeliverNewSegment((OutputPin *)This->ppPins[i], 0, (LONGLONG)ceil(10000000.0 * (float)((AVISplitter_OutputPin *)This->ppPins[i])->dwLength / ((AVISplitter_OutputPin *)This->ppPins[i])->fSamplesPerSec), 1.0);
                ((AVISplitter_OutputPin *)This->ppPins[i])->mediaSeeking.llDuration = (LONGLONG)ceil(10000000.0 * (float)((AVISplitter_OutputPin *)This->ppPins[i])->dwLength / ((AVISplitter_OutputPin *)This->ppPins[i])->fSamplesPerSec);
                ((AVISplitter_OutputPin *)This->ppPins[i])->mediaSeeking.llStop = (LONGLONG)ceil(10000000.0 * (float)((AVISplitter_OutputPin *)This->ppPins[i])->dwLength / ((AVISplitter_OutputPin *)This->ppPins[i])->fSamplesPerSec);
                OutputPin_CommitAllocator((OutputPin *)This->ppPins[i]);
            }

            /* FIXME: this is a little hacky: we have to deliver (at least?) one sample
             * to each renderer before they will complete their transitions. We should probably
             * seek through the stream for the first of each, rather than do it this way which is
             * probably a bit prone to deadlocking */
            hr = PullPin_StartProcessing(This->pInputPin);
        }
    }
    /* FIXME: else pause thread */

    return hr;
}

static HRESULT WINAPI AVISplitter_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%s)\n", wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        This->rtStreamStart = tStart;

        This->state = State_Running;
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI AVISplitter_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%ld, %p)\n", dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    /* FIXME: this is a little bit unsafe, but I don't see that we can do this
     * while in the critical section. Maybe we could copy the pointer and addref in the
     * critical section and then release after this.
     */
    if (This->pInputPin && (PullPin_WaitForStateChange(This->pInputPin, dwMilliSecsTimeout) == S_FALSE))
        return VFW_S_STATE_INTERMEDIATE;

    return S_OK;
}

static HRESULT WINAPI AVISplitter_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%p)\n", pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI AVISplitter_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%p)\n", ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI AVISplitter_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%p)\n", ppEnum);

    epd.cPins = This->cStreams + 1; /* +1 for input pin */
    epd.ppPins = This->ppPins;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI AVISplitter_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("AVISplitter::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI AVISplitter_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%p)\n", pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    
    return S_OK;
}

static HRESULT WINAPI AVISplitter_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    HRESULT hr = S_OK;
    ICOM_THIS(AVISplitter, iface);

    TRACE("(%p, %s)\n", pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            strcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI AVISplitter_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    TRACE("(%p)\n", pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AVISplitter_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    AVISplitter_QueryInterface,
    AVISplitter_AddRef,
    AVISplitter_Release,
    AVISplitter_GetClassID,
    AVISplitter_Stop,
    AVISplitter_Pause,
    AVISplitter_Run,
    AVISplitter_GetState,
    AVISplitter_SetSyncSource,
    AVISplitter_GetSyncSource,
    AVISplitter_EnumPins,
    AVISplitter_FindPin,
    AVISplitter_QueryFilterInfo,
    AVISplitter_JoinFilterGraph,
    AVISplitter_QueryVendorInfo
};

static HRESULT AVISplitter_NextChunk(LONGLONG * pllCurrentChunkOffset, RIFFCHUNK * pCurrentChunk, const REFERENCE_TIME * tStart, const REFERENCE_TIME * tStop, const BYTE * pbSrcStream)
{
    *pllCurrentChunkOffset += MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK) + RIFFROUND(pCurrentChunk->cb));

    if (*pllCurrentChunkOffset > *tStop)
        return S_FALSE; /* no more data - we couldn't even get the next chunk header! */
    else if (*pllCurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) >= *tStop)
    {
        memcpy(pCurrentChunk, pbSrcStream + (DWORD)BYTES_FROM_MEDIATIME(*pllCurrentChunkOffset - *tStart), (DWORD)BYTES_FROM_MEDIATIME(*tStop - *pllCurrentChunkOffset));
        return S_FALSE; /* no more data */
    }
    else
        memcpy(pCurrentChunk, pbSrcStream + (DWORD)BYTES_FROM_MEDIATIME(*pllCurrentChunkOffset - *tStart), sizeof(RIFFCHUNK));

    return S_OK;
}

static HRESULT AVISplitter_Sample(LPVOID iface, IMediaSample * pSample)
{
    ICOM_THIS(AVISplitter, iface);
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    BOOL bMoreData = TRUE;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* trace removed for performance reasons */
/*  TRACE("(%p)\n", pSample); */

    assert(BYTES_FROM_MEDIATIME(tStop - tStart) == cbSrcStream);

    if (This->CurrentChunkOffset <= tStart && This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) > tStart)
    {
        DWORD offset = (DWORD)BYTES_FROM_MEDIATIME(tStart - This->CurrentChunkOffset);
        assert(offset <= sizeof(RIFFCHUNK));
        memcpy((BYTE *)&This->CurrentChunk + offset, pbSrcStream, sizeof(RIFFCHUNK) - offset);
    }
    else if (This->CurrentChunkOffset > tStart)
    {
        DWORD offset = (DWORD)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart);
        if (offset >= (DWORD)cbSrcStream)
        {
            FIXME("large offset\n");
            return S_OK;
        }

        memcpy(&This->CurrentChunk, pbSrcStream + offset, sizeof(RIFFCHUNK));
    }

    assert(This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) < tStop);

    while (bMoreData)
    {
        BYTE * pbDstStream;
        long cbDstStream;
        long chunk_remaining_bytes = 0;
        long offset_src;
        WORD streamId;
        AVISplitter_OutputPin * pOutputPin;
        BOOL bSyncPoint = TRUE;

        if (This->CurrentChunkOffset >= tStart)
            offset_src = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart) + sizeof(RIFFCHUNK);
        else
            offset_src = 0;

        switch (TWOCCFromFOURCC(This->CurrentChunk.fcc))
        {
        case cktypeDIBcompressed:
            bSyncPoint = FALSE;
            /* fall-through */
        case cktypeDIBbits:
            /* FIXME: check that pin is of type video */
            break;
        case cktypeWAVEbytes:
            /* FIXME: check that pin is of type audio */
            break;
        case cktypePALchange:
            FIXME("handle palette change\n");
            break;
        case ckidJUNK:
            /* silently ignore */
            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream))
                bMoreData = FALSE;
            continue;
        default:
            FIXME("Skipping unknown chunk type: %s at file offset 0x%lx\n", debugstr_an((LPSTR)&This->CurrentChunk.fcc, 4), (DWORD)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset));
            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream))
                bMoreData = FALSE;
            continue;
        }

        streamId = StreamFromFOURCC(This->CurrentChunk.fcc);

        if (streamId > This->cStreams)
        {
            ERR("Corrupted AVI file (contains stream id %d, but supposed to only have %ld streams)\n", streamId, This->cStreams);
            return E_FAIL;
        }

        pOutputPin = (AVISplitter_OutputPin *)This->ppPins[streamId + 1];

        if (!This->pCurrentSample)
        {
            /* cache media sample until it is ready to be despatched
             * (i.e. we reach the end of the chunk) */
            hr = OutputPin_GetDeliveryBuffer(&pOutputPin->pin, &This->pCurrentSample, NULL, NULL, 0);

            if (SUCCEEDED(hr))
            {
                hr = IMediaSample_SetActualDataLength(This->pCurrentSample, 0);
                assert(hr == S_OK);
            }
            else
            {
                TRACE("Skipping sending sample for stream %02d due to error (%lx)\n", streamId, hr);
                This->pCurrentSample = NULL;
                if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream))
                    bMoreData = FALSE;
                continue;
            }
        }

        hr = IMediaSample_GetPointer(This->pCurrentSample, &pbDstStream);

        if (SUCCEEDED(hr))
        {
            cbDstStream = IMediaSample_GetSize(This->pCurrentSample);

            chunk_remaining_bytes = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(This->CurrentChunk.cb + sizeof(RIFFCHUNK)) - tStart) - offset_src;
        
            assert(chunk_remaining_bytes >= 0);
            assert(chunk_remaining_bytes <= cbDstStream - IMediaSample_GetActualDataLength(This->pCurrentSample));

            /* trace removed for performance reasons */
/*          TRACE("chunk_remaining_bytes: 0x%lx, cbSrcStream: 0x%lx, offset_src: 0x%lx\n", chunk_remaining_bytes, cbSrcStream, offset_src); */
        }

        if (chunk_remaining_bytes <= cbSrcStream - offset_src)
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + IMediaSample_GetActualDataLength(This->pCurrentSample), pbSrcStream + offset_src, chunk_remaining_bytes);
                hr = IMediaSample_SetActualDataLength(This->pCurrentSample, chunk_remaining_bytes + IMediaSample_GetActualDataLength(This->pCurrentSample));
                assert(hr == S_OK);
            }

            if (SUCCEEDED(hr))
            {
                REFERENCE_TIME tAviStart, tAviStop;

                /* FIXME: hack */
                if (pOutputPin->dwSamplesProcessed == 0)
                    IMediaSample_SetDiscontinuity(This->pCurrentSample, TRUE);

                IMediaSample_SetSyncPoint(This->pCurrentSample, bSyncPoint);

                pOutputPin->dwSamplesProcessed++;

                if (pOutputPin->dwSampleSize)
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)pOutputPin->dwSampleSize * pOutputPin->fSamplesPerSec));
                else
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) / (float)pOutputPin->fSamplesPerSec);
                if (pOutputPin->dwSampleSize)
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)pOutputPin->dwSampleSize * pOutputPin->fSamplesPerSec));
                else
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed / (float)pOutputPin->fSamplesPerSec);

                IMediaSample_SetTime(This->pCurrentSample, &tAviStart, &tAviStop);


                hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);
                if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
                    ERR("Error sending sample (%lx)\n", hr);
            }

            if (This->pCurrentSample)
                IMediaSample_Release(This->pCurrentSample);
            
            This->pCurrentSample = NULL;

            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream))
                bMoreData = FALSE;
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + IMediaSample_GetActualDataLength(This->pCurrentSample), pbSrcStream + offset_src, cbSrcStream - offset_src);
                IMediaSample_SetActualDataLength(This->pCurrentSample, cbSrcStream - offset_src + IMediaSample_GetActualDataLength(This->pCurrentSample));
            }
            bMoreData = FALSE;
        }
    }
    return hr;
}

static HRESULT AVISplitter_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream) && IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_Avi))
        return S_OK;
    return S_FALSE;
}

static HRESULT AVISplitter_ProcessStreamList(AVISplitter * This, const BYTE * pData, DWORD cb)
{
    PIN_INFO piOutput;
    RIFFCHUNK * pChunk;
    IPin ** ppOldPins;
    HRESULT hr;
    AM_MEDIA_TYPE amt;
    float fSamplesPerSec = 0.0f;
    DWORD dwSampleSize = 0;
    DWORD dwLength = 0;
    ALLOCATOR_PROPERTIES props;
    static const WCHAR wszStreamTemplate[] = {'S','t','r','e','a','m',' ','%','0','2','d',0};

    props.cbAlign = 1;
    props.cbPrefix = 0;
    props.cbBuffer = 0x20000;
    props.cBuffers = 2;
    
    ZeroMemory(&amt, sizeof(amt));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    wsprintfW(piOutput.achName, wszStreamTemplate, This->cStreams);

    for (pChunk = (RIFFCHUNK *)pData; ((BYTE *)pChunk >= pData) && ((BYTE *)pChunk + sizeof(RIFFCHUNK) < pData + cb) && (pChunk->cb > 0); pChunk = (RIFFCHUNK *)((BYTE*)pChunk + sizeof(RIFFCHUNK) + pChunk->cb))
    {
        switch (pChunk->fcc)
        {
        case ckidSTREAMHEADER:
            {
                const AVISTREAMHEADER * pStrHdr = (const AVISTREAMHEADER *)pChunk;
                TRACE("processing stream header\n");

                fSamplesPerSec = (float)pStrHdr->dwRate / (float)pStrHdr->dwScale;

                switch (pStrHdr->fccType)
                {
                case streamtypeVIDEO:
                    memcpy(&amt.formattype, &FORMAT_VideoInfo, sizeof(GUID));
                    amt.pbFormat = NULL;
                    amt.cbFormat = 0;
                    break;
                case streamtypeAUDIO:
                    memcpy(&amt.formattype, &FORMAT_WaveFormatEx, sizeof(GUID));
                    break;
                default:
                    memcpy(&amt.formattype, &FORMAT_None, sizeof(GUID));
                }
                memcpy(&amt.majortype, &MEDIATYPE_Video, sizeof(GUID));
                amt.majortype.Data1 = pStrHdr->fccType;
                memcpy(&amt.subtype, &MEDIATYPE_Video, sizeof(GUID));
                amt.subtype.Data1 = pStrHdr->fccHandler;
                TRACE("Subtype FCC: %.04s\n", (LPSTR)&pStrHdr->fccHandler);
                amt.lSampleSize = pStrHdr->dwSampleSize;
                amt.bFixedSizeSamples = (amt.lSampleSize != 0);

                /* FIXME: Is this right? */
                if (!amt.lSampleSize)
                {
                    amt.lSampleSize = 1;
                    dwSampleSize = 1;
                }

                amt.bTemporalCompression = IsEqualGUID(&amt.majortype, &MEDIATYPE_Video); /* FIXME? */
                dwSampleSize = pStrHdr->dwSampleSize;
                dwLength = pStrHdr->dwLength;
                if (!dwLength)
                    dwLength = This->AviHeader.dwTotalFrames;

                if (pStrHdr->dwSuggestedBufferSize)
                    props.cbBuffer = pStrHdr->dwSuggestedBufferSize;

                break;
            }
        case ckidSTREAMFORMAT:
            TRACE("processing stream format data\n");
            if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo))
            {
                VIDEOINFOHEADER * pvi;
                /* biCompression member appears to override the value in the stream header.
                 * i.e. the stream header can say something completely contradictory to what
                 * is in the BITMAPINFOHEADER! */
                if (pChunk->cb < sizeof(BITMAPINFOHEADER))
                {
                    ERR("Not enough bytes for BITMAPINFOHEADER\n");
                    return E_FAIL;
                }
                amt.cbFormat = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                ZeroMemory(amt.pbFormat, amt.cbFormat);
                pvi = (VIDEOINFOHEADER *)amt.pbFormat;
                pvi->AvgTimePerFrame = (LONGLONG)(10000000.0 / fSamplesPerSec);
                CopyMemory(&pvi->bmiHeader, (const BYTE *)(pChunk + 1), pChunk->cb);
                if (pvi->bmiHeader.biCompression)
                    amt.subtype.Data1 = pvi->bmiHeader.biCompression;
            }
            else
            {
                amt.cbFormat = pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                CopyMemory(amt.pbFormat, (const BYTE *)(pChunk + 1), amt.cbFormat);
            }
            break;
        case ckidSTREAMNAME:
            TRACE("processing stream name\n");
            /* FIXME: this doesn't exactly match native version (we omit the "##)" prefix), but hey... */
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(pChunk + 1), pChunk->cb, piOutput.achName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
            break;
        case ckidSTREAMHANDLERDATA:
            FIXME("process stream handler data\n");
            break;
        case ckidJUNK:
            TRACE("JUNK chunk ignored\n");
            break;
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPSTR)&pChunk->fcc);
        }
    }

    if (IsEqualGUID(&amt.formattype, &FORMAT_WaveFormatEx))
    {
        memcpy(&amt.subtype, &MEDIATYPE_Video, sizeof(GUID));
        amt.subtype.Data1 = ((WAVEFORMATEX *)amt.pbFormat)->wFormatTag;
    }

    dump_AM_MEDIA_TYPE(&amt);
    FIXME("fSamplesPerSec = %f\n", (double)fSamplesPerSec);
    FIXME("dwSampleSize = %lx\n", dwSampleSize);
    FIXME("dwLength = %lx\n", dwLength);

    ppOldPins = This->ppPins;

    This->ppPins = HeapAlloc(GetProcessHeap(), 0, (This->cStreams + 2) * sizeof(IPin *));
    memcpy(This->ppPins, ppOldPins, (This->cStreams + 1) * sizeof(IPin *));

    hr = AVISplitter_OutputPin_Construct(&piOutput, &props, NULL, AVISplitter_OutputPin_QueryAccept, &amt, fSamplesPerSec, &This->csFilter, This->ppPins + This->cStreams + 1);

    if (SUCCEEDED(hr))
    {
        ((AVISplitter_OutputPin *)(This->ppPins[This->cStreams + 1]))->dwSampleSize = dwSampleSize;
        ((AVISplitter_OutputPin *)(This->ppPins[This->cStreams + 1]))->dwLength = dwLength;
        ((AVISplitter_OutputPin *)(This->ppPins[This->cStreams + 1]))->pin.pin.pUserData = (LPVOID)This->ppPins[This->cStreams + 1];
        This->cStreams++;
        HeapFree(GetProcessHeap(), 0, ppOldPins);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, This->ppPins);
        This->ppPins = ppOldPins;
        ERR("Failed with error %lx\n", hr);
    }

    return hr;
}

static HRESULT AVISplitter_RemoveOutputPins(AVISplitter * This)
{
    /* NOTE: should be in critical section when calling this function */

    ULONG i;
    IPin ** ppOldPins = This->ppPins;

    /* reduce the pin array down to 1 (just our input pin) */
    This->ppPins = HeapAlloc(GetProcessHeap(), 0, sizeof(IPin *) * 1);
    memcpy(This->ppPins, ppOldPins, sizeof(IPin *) * 1);

    for (i = 0; i < This->cStreams; i++)
    {
        OutputPin_DeliverDisconnect((OutputPin *)ppOldPins[i + 1]);
        IPin_Release(ppOldPins[i + 1]);
    }

    This->cStreams = 0;
    HeapFree(GetProcessHeap(), 0, ppOldPins);

    return S_OK;
}

/* FIXME: fix leaks on failure here */
static HRESULT AVISplitter_InputPin_PreConnect(IPin * iface, IPin * pConnectPin)
{
    ICOM_THIS(PullPin, iface);
    HRESULT hr;
    RIFFLIST list;
    LONGLONG pos = 0; /* in bytes */
    BYTE * pBuffer;
    RIFFCHUNK * pCurrentChunk;
    AVISplitter * pAviSplit = (AVISplitter *)This->pin.pinInfo.pFilter;

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

    if (list.fcc != ckidRIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.cb > 1 * 1024 * 1024 * 1024) /* cannot be more than 1Gb in size */
    {
        ERR("Input stream violates RIFF spec\n");
        return E_FAIL;
    }
    if (list.fccListType != ckidAVI)
    {
        ERR("Input stream not an AVI RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    if (list.fcc != ckidLIST)
    {
        ERR("Expected LIST chunk, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != ckidHEADERLIST)
    {
        ERR("Header list expected. Got: %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK));
    hr = IAsyncReader_SyncRead(This->pReader, pos + sizeof(list), list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK), pBuffer);

    pAviSplit->AviHeader.cb = 0;

    for (pCurrentChunk = (RIFFCHUNK *)pBuffer; (BYTE *)pCurrentChunk + sizeof(*pCurrentChunk) < pBuffer + list.cb; pCurrentChunk = (RIFFCHUNK *)(((BYTE *)pCurrentChunk) + sizeof(*pCurrentChunk) + pCurrentChunk->cb))
    {
        RIFFLIST * pList;

        switch (pCurrentChunk->fcc)
        {
        case ckidMAINAVIHEADER:
            /* AVIMAINHEADER includes the structure that is pCurrentChunk at the moment */
            memcpy(&pAviSplit->AviHeader, pCurrentChunk, sizeof(pAviSplit->AviHeader));
            break;
        case ckidLIST:
            pList = (RIFFLIST *)pCurrentChunk;
            switch (pList->fccListType)
            {
            case ckidSTREAMLIST:
                hr = AVISplitter_ProcessStreamList(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST));
                break;
            case ckidODML:
                FIXME("process ODML header\n");
                break;
            }
            break;
        case ckidJUNK:
            /* ignore */
            break;
        default:
            FIXME("unrecognised header list type: %.04s\n", (LPSTR)&pCurrentChunk->fcc);
        }
    }
    HeapFree(GetProcessHeap(), 0, pBuffer);

    if (pAviSplit->AviHeader.cb != sizeof(pAviSplit->AviHeader) - sizeof(RIFFCHUNK))
    {
        ERR("Avi Header wrong size!\n");
        return E_FAIL;
    }

    pos += sizeof(RIFFCHUNK) + list.cb;
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);

    if (list.fcc == ckidJUNK)
    {
        pos += sizeof(RIFFCHUNK) + list.cb;
        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    }

    if (list.fcc != ckidLIST)
    {
        ERR("Expected LIST, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != ckidAVIMOVIE)
    {
        ERR("Expected AVI movie list, but got %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    if (hr == S_OK)
    {
        pAviSplit->CurrentChunkOffset = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFLIST));
        pAviSplit->EndOfFile = MEDIATIME_FROM_BYTES(pos + list.cb + sizeof(RIFFLIST));
        hr = IAsyncReader_SyncRead(This->pReader, BYTES_FROM_MEDIATIME(pAviSplit->CurrentChunkOffset), sizeof(pAviSplit->CurrentChunk), (BYTE *)&pAviSplit->CurrentChunk);
    }

    if (hr != S_OK)
        return E_FAIL;

    TRACE("AVI File ok\n");

    return hr;
}

static HRESULT AVISplitter_ChangeStart(LPVOID iface)
{
    FIXME("(%p)\n", iface);
    return S_OK;
}

static HRESULT AVISplitter_ChangeStop(LPVOID iface)
{
    FIXME("(%p)\n", iface);
    return S_OK;
}

static HRESULT AVISplitter_ChangeRate(LPVOID iface)
{
    FIXME("(%p)\n", iface);
    return S_OK;
}


static HRESULT WINAPI AVISplitter_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_From_IMediaSeeking(AVISplitter_OutputPin, iface);

    return IUnknown_QueryInterface((IUnknown *)This, riid, ppv);
}

static ULONG WINAPI AVISplitter_Seeking_AddRef(IMediaSeeking * iface)
{
    ICOM_THIS_From_IMediaSeeking(AVISplitter_OutputPin, iface);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI AVISplitter_Seeking_Release(IMediaSeeking * iface)
{
    ICOM_THIS_From_IMediaSeeking(AVISplitter_OutputPin, iface);

    return IUnknown_Release((IUnknown *)This);
}

static const IMediaSeekingVtbl AVISplitter_Seeking_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    AVISplitter_Seeking_QueryInterface,
    AVISplitter_Seeking_AddRef,
    AVISplitter_Seeking_Release,
    MediaSeekingImpl_GetCapabilities,
    MediaSeekingImpl_CheckCapabilities,
    MediaSeekingImpl_IsFormatSupported,
    MediaSeekingImpl_QueryPreferredFormat,
    MediaSeekingImpl_GetTimeFormat,
    MediaSeekingImpl_IsUsingTimeFormat,
    MediaSeekingImpl_SetTimeFormat,
    MediaSeekingImpl_GetDuration,
    MediaSeekingImpl_GetStopPosition,
    MediaSeekingImpl_GetCurrentPosition,
    MediaSeekingImpl_ConvertTimeFormat,
    MediaSeekingImpl_SetPositions,
    MediaSeekingImpl_GetPositions,
    MediaSeekingImpl_GetAvailable,
    MediaSeekingImpl_SetRate,
    MediaSeekingImpl_GetRate,
    MediaSeekingImpl_GetPreroll
};

HRESULT WINAPI AVISplitter_OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS(AVISplitter_OutputPin, iface);

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = (LPVOID)&This->mediaSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI AVISplitter_OutputPin_Release(IPin * iface)
{
    ICOM_THIS(AVISplitter_OutputPin, iface);
    
    TRACE("()\n");
    
    if (!InterlockedDecrement(&This->pin.pin.refCount))
    {
        DeleteMediaType(This->pmt);
        CoTaskMemFree(This->pmt);
        DeleteMediaType(&This->pin.pin.mtCurrent);
        CoTaskMemFree(This);
        return 0;
    }
    return This->pin.pin.refCount;
}

static HRESULT WINAPI AVISplitter_OutputPin_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    ENUMMEDIADETAILS emd;
    ICOM_THIS(AVISplitter_OutputPin, iface);

    TRACE("(%p)\n", ppEnum);

    /* override this method to allow enumeration of your types */
    emd.cMediaTypes = 1;
    emd.pMediaTypes = This->pmt;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

static HRESULT AVISplitter_OutputPin_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    ICOM_THIS(AVISplitter_OutputPin, iface);

    TRACE("()\n");
    dump_AM_MEDIA_TYPE(pmt);

    return (memcmp(This->pmt, pmt, sizeof(AM_MEDIA_TYPE)) == 0);
}

static const IPinVtbl AVISplitter_OutputPin_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    AVISplitter_OutputPin_QueryInterface,
    IPinImpl_AddRef,
    AVISplitter_OutputPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    OutputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    AVISplitter_OutputPin_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

static HRESULT AVISplitter_InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    PullPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(PullPin_Init(pPinInfo, pSampleProc, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &AVISplitter_InputPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI AVISplitter_InputPin_Disconnect(IPin * iface)
{
    HRESULT hr;
    ICOM_THIS(IPinImpl, iface);

    TRACE("()\n");

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            FILTER_STATE state;

            hr = IBaseFilter_GetState(This->pinInfo.pFilter, 0, &state);

            if (SUCCEEDED(hr) && (state == State_Stopped))
            {
                IPin_Release(This->pConnectedTo);
                This->pConnectedTo = NULL;
                hr = AVISplitter_RemoveOutputPins((AVISplitter *)This->pinInfo.pFilter);
            }
            else
                hr = VFW_E_NOT_STOPPED;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);
    
    return hr;
}

static const IPinVtbl AVISplitter_InputPin_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    PullPin_QueryInterface,
    IPinImpl_AddRef,
    PullPin_Release,
    OutputPin_Connect,
    PullPin_ReceiveConnection,
    AVISplitter_InputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
