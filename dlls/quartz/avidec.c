/*
 * AVI Decompressor (VFW decompressors wrapper)
 *
 * Copyright 2004 Christian Costa
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

#include "config.h"

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "aviriff.h"
#include "mmreg.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "vfw.h"
/* #include "fourcc.h" */
/* #include "avcodec.h" */

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const WCHAR wcsOutputPinName[] = {'o','u','t','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl AVIDec_Vtbl;
static const IPinVtbl AVIDec_InputPin_Vtbl;
static const IMemInputPinVtbl MemInputPin_Vtbl; 
static const IPinVtbl AVIDec_OutputPin_Vtbl;

typedef struct AVIDecImpl
{
    const IBaseFilterVtbl * lpVtbl;

    ULONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    IPin ** ppPins;

    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
    int init;
} AVIDecImpl;

static DWORD AVIDec_SendSampleData(AVIDecImpl* This, LPBYTE data, DWORD size)
{
    VIDEOINFOHEADER* format;
    AM_MEDIA_TYPE amt;
    HRESULT hr;
    DWORD res;
    IMediaSample* pSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;

    hr = IPin_ConnectionMediaType(This->ppPins[0], &amt);
    if (FAILED(hr)) {
	ERR("Unable to retrieve media type\n");
	goto error;
    }
    format = (VIDEOINFOHEADER*)amt.pbFormat;

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = size;

    hr = OutputPin_GetDeliveryBuffer((OutputPin*)This->ppPins[1], &pSample, NULL, NULL, 0);
    if (FAILED(hr)) {
	ERR("Unable to get delivery buffer (%lx)\n", hr);
	goto error;
    }
    
    hr = IMediaSample_SetActualDataLength(pSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pSample, &pbDstStream);
    if (FAILED(hr)) {
	ERR("Unable to get pointer to buffer (%lx)\n", hr);
	goto error;
    }
    cbDstStream = IMediaSample_GetSize(pSample);
    if (cbDstStream < This->pBihOut->biSizeImage) {
        ERR("Sample size is too small %ld < %ld\n", cbDstStream, This->pBihOut->biSizeImage);
	hr = E_FAIL;
	goto error;
    }

    res = ICDecompress(This->hvid, 0, This->pBihIn, data, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%lx)\n", res);

    hr = OutputPin_SendSample((OutputPin*)This->ppPins[1], pSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
        ERR("Error sending sample (%lx)\n", hr);
	goto error;
    }

error:
    if (pSample)
        IMediaSample_Release(pSample);

    return hr;
}

static HRESULT AVIDec_Sample(LPVOID iface, IMediaSample * pSample)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);
    
    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%lx)\n", hr);
	return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        ERR("Cannot get sample time (%lx)\n", hr);
    
    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %ld\n", pbSrcStream, cbSrcStream);

#if 0 /* For debugging purpose */
    {
        int i;
        for(i = 0; i < cbSrcStream; i++)
        {
	    if ((i!=0) && !(i%16))
	        DPRINTF("\n");
	    DPRINTF("%02x ", pbSrcStream[i]);
        }
        DPRINTF("\n");
    }
#endif
    
    AVIDec_SendSampleData(This, pbSrcStream, cbSrcStream);

    return S_OK;
}

static HRESULT AVIDec_Input_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* pAVIDec = (AVIDecImpl*)iface;
    TRACE("%p\n", iface);
    dump_AM_MEDIA_TYPE(pmt);

    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((char*)&pmt->subtype)+4, ((char*)&MEDIATYPE_Video)+4, sizeof(GUID)-4)) && /* Check root (GUID w/o FOURCC) */
        (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)))
    {
        HIC drv;
        VIDEOINFOHEADER* format = (VIDEOINFOHEADER*)pmt->pbFormat;
        drv = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, &format->bmiHeader, NULL, ICMODE_DECOMPRESS);
        if (drv)
        {
            AM_MEDIA_TYPE* outpmt = &((OutputPin*)pAVIDec->ppPins[1])->pin.mtCurrent;
            const CLSID* outsubtype;
            DWORD bih_size;

            switch(format->bmiHeader.biBitCount)
            {
                case 32: outsubtype = &MEDIASUBTYPE_RGB32; break;
                case 24: outsubtype = &MEDIASUBTYPE_RGB24; break;
                case 16: outsubtype = &MEDIASUBTYPE_RGB565; break;
                case 8:  outsubtype = &MEDIASUBTYPE_RGB8; break;
                default:
                    FIXME("Depth %d not supported\n", format->bmiHeader.biBitCount);
                    ICClose(drv);
                    return S_FALSE;
            }
            CopyMediaType( outpmt, pmt);
            outpmt->subtype = *outsubtype;
            pAVIDec->hvid = drv;

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            if (pAVIDec->pBihIn) {
                CoTaskMemFree(pAVIDec->pBihIn);
                CoTaskMemFree(pAVIDec->pBihOut);
            }
            bih_size = format->bmiHeader.biSize + format->bmiHeader.biClrUsed * 4;
            pAVIDec->pBihIn = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!pAVIDec->pBihIn)
            {
                ICClose(drv);
                return E_OUTOFMEMORY;
            }
            pAVIDec->pBihOut = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!pAVIDec->pBihOut)
            {
                CoTaskMemFree(pAVIDec->pBihIn);
                pAVIDec->pBihIn = NULL;
                ICClose(drv);
                return E_OUTOFMEMORY;
            }
            memcpy(pAVIDec->pBihIn, &format->bmiHeader, bih_size);
            memcpy(pAVIDec->pBihOut, &format->bmiHeader, bih_size);

            /* Update output format as non compressed bitmap */
            pAVIDec->pBihOut->biCompression = 0;
            pAVIDec->pBihOut->biSizeImage = pAVIDec->pBihOut->biWidth * pAVIDec->pBihOut->biHeight * pAVIDec->pBihOut->biBitCount / 8;

            /* Update buffer size of media samples in output */
            ((OutputPin*)pAVIDec->ppPins[1])->allocProps.cbBuffer = pAVIDec->pBihOut->biSizeImage;
	    
            pAVIDec->init = 1;
            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }
    
    TRACE("Connection refused\n");
    return S_FALSE;
}


static HRESULT AVIDec_Output_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* pAVIDec = (AVIDecImpl*)iface;
    AM_MEDIA_TYPE* outpmt = &((OutputPin*)pAVIDec->ppPins[1])->pin.mtCurrent;
    TRACE("%p\n", iface);

    if (IsEqualIID(&pmt->majortype, &outpmt->majortype) && IsEqualIID(&pmt->subtype, &outpmt->subtype))
        return S_OK;
    return S_FALSE;
}

static HRESULT AVIDec_InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    InputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;
    TRACE("QA : %p %p\n", pQueryAccept, AVIDec_Input_QueryAccept);
    if (SUCCEEDED(InputPin_Init(pPinInfo, pSampleProc, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &AVIDec_InputPin_Vtbl;
        pPinImpl->lpVtblMemInput = &MemInputPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT AVIDec_OutputPin_Construct(const PIN_INFO * pPinInfo, ALLOCATOR_PROPERTIES *props, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    OutputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_OUTPUT)
    {
        ERR("Pin direction(%x) != PINDIR_OUTPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(OutputPin_Init(pPinInfo, props, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &AVIDec_OutputPin_Vtbl;
        
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    PIN_INFO piOutput;
    AVIDecImpl * pAVIDec;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    pAVIDec = CoTaskMemAlloc(sizeof(AVIDecImpl));

    pAVIDec->lpVtbl = &AVIDec_Vtbl;
    
    pAVIDec->refCount = 1;
    InitializeCriticalSection(&pAVIDec->csFilter);
    pAVIDec->state = State_Stopped;
    pAVIDec->pClock = NULL;
    pAVIDec->pBihIn = NULL;
    pAVIDec->pBihOut = NULL;
    pAVIDec->init = 0;
    ZeroMemory(&pAVIDec->filterInfo, sizeof(FILTER_INFO));

    pAVIDec->ppPins = CoTaskMemAlloc(2 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pAVIDec;
    strncpyW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)pAVIDec;
    strncpyW(piOutput.achName, wcsOutputPinName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));

    hr = AVIDec_InputPin_Construct(&piInput, AVIDec_Sample, (LPVOID)pAVIDec, AVIDec_Input_QueryAccept, &pAVIDec->csFilter, &pAVIDec->ppPins[0]);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES props;
        props.cbAlign = 1;
        props.cbPrefix = 0;
        props.cbBuffer = 0; /* Will be updated at connection time */
        props.cBuffers = 2;
	
        hr = AVIDec_OutputPin_Construct(&piOutput, &props, NULL, AVIDec_Output_QueryAccept, &pAVIDec->csFilter, &pAVIDec->ppPins[1]);

	if (FAILED(hr))
	    ERR("Cannot create output pin (%lx)\n", hr);
	
        *ppv = (LPVOID)pAVIDec;
    }
    else
    {
        CoTaskMemFree(pAVIDec->ppPins);
        DeleteCriticalSection(&pAVIDec->csFilter);
        CoTaskMemFree(pAVIDec);
    }

    return hr;
}

static HRESULT WINAPI AVIDec_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

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

static ULONG WINAPI AVIDec_AddRef(IBaseFilter * iface)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->() AddRef from %ld\n", This, iface, This->refCount);

    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI AVIDec_Release(IBaseFilter * iface)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->() Release from %ld\n", This, iface, This->refCount);

    if (!InterlockedDecrement(&This->refCount))
    {
        ULONG i;

        DeleteCriticalSection(&This->csFilter);
        IReferenceClock_Release(This->pClock);
        
        for (i = 0; i < 2; i++)
            IPin_Release(This->ppPins[i]);
        
        HeapFree(GetProcessHeap(), 0, This->ppPins);
        This->lpVtbl = NULL;

	if (This->hvid)
            ICClose(This->hvid);
	
        if (This->pBihIn) {
            CoTaskMemFree(This->pBihIn);
            CoTaskMemFree(This->pBihOut);
        }
	    
        TRACE("Destroying AVI Decompressor\n");
        CoTaskMemFree(This);
        
        return 0;
    }
    else
        return This->refCount;
}

/** IPersist methods **/

static HRESULT WINAPI AVIDec_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = CLSID_AVIDec;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI AVIDec_Stop(IBaseFilter * iface)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

static HRESULT WINAPI AVIDec_Pause(IBaseFilter * iface)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;
    
    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI AVIDec_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        This->rtStreamStart = tStart;
        This->state = State_Running;
        OutputPin_CommitAllocator((OutputPin *)This->ppPins[1]);
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI AVIDec_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%ld, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI AVIDec_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

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

static HRESULT WINAPI AVIDec_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI AVIDec_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    epd.cPins = 2; /* input and output pins */
    epd.ppPins = This->ppPins;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI AVIDec_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    FIXME("AVISplitter::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI AVIDec_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    
    return S_OK;
}

static HRESULT WINAPI AVIDec_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    HRESULT hr = S_OK;
    AVIDecImpl *This = (AVIDecImpl *)iface;

    TRACE("(%p/%p)->(%p, %s)\n", This, iface, pGraph, debugstr_w(pName));

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

static HRESULT WINAPI AVIDec_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    AVIDecImpl *This = (AVIDecImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AVIDec_Vtbl =
{
    AVIDec_QueryInterface,
    AVIDec_AddRef,
    AVIDec_Release,
    AVIDec_GetClassID,
    AVIDec_Stop,
    AVIDec_Pause,
    AVIDec_Run,
    AVIDec_GetState,
    AVIDec_SetSyncSource,
    AVIDec_GetSyncSource,
    AVIDec_EnumPins,
    AVIDec_FindPin,
    AVIDec_QueryFilterInfo,
    AVIDec_JoinFilterGraph,
    AVIDec_QueryVendorInfo
};

static const IPinVtbl AVIDec_InputPin_Vtbl = 
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    InputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};

HRESULT WINAPI AVIDec_Output_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    IPinImpl *This = (IPinImpl *)iface;
    ENUMMEDIADETAILS emd;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    emd.cMediaTypes = 1;
    emd.pMediaTypes = &This->mtCurrent;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

HRESULT WINAPI AVIDec_Output_Disconnect(IPin * iface)
{
    OutputPin* This = (OutputPin*)iface;
    HRESULT hr;
    AVIDecImpl* pAVIDec = (AVIDecImpl*)This->pin.pinInfo.pFilter;

    TRACE("(%p/%p)->()\n", This, iface);
    
    hr = OutputPin_Disconnect(iface);

    if (hr == S_OK)
    {
        ICClose(pAVIDec->hvid);
	pAVIDec->hvid = 0;
    }
    
    return hr;
}

static const IPinVtbl AVIDec_OutputPin_Vtbl = 
{
    OutputPin_QueryInterface,
    IPinImpl_AddRef,
    OutputPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    AVIDec_Output_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    AVIDec_Output_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

static const IMemInputPinVtbl MemInputPin_Vtbl = 
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};
