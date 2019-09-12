/*
 * Parser (Base for parsers and splitters)
 *
 * Copyright 2003 Robert Shearman
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
#include "pin.h"

#include "vfwmsgs.h"
#include "amvideo.h"

#include "wine/debug.h"

#include <math.h>
#include <assert.h>

#include "parser.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IMediaSeekingVtbl Parser_Seeking_Vtbl;
static const IPinVtbl Parser_OutputPin_Vtbl;
static const IPinVtbl Parser_InputPin_Vtbl;

static HRESULT WINAPI Parser_ChangeStart(IMediaSeeking *iface);
static HRESULT WINAPI Parser_ChangeStop(IMediaSeeking *iface);
static HRESULT WINAPI Parser_ChangeRate(IMediaSeeking *iface);
static HRESULT WINAPI Parser_OutputPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props);
static HRESULT WINAPI Parser_OutputPin_CheckMediaType(BasePin *pin, const AM_MEDIA_TYPE *pmt);
static HRESULT WINAPI Parser_OutputPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt);
static HRESULT WINAPI Parser_OutputPin_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator);

static inline ParserImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return CONTAINING_RECORD(iface, ParserImpl, sourceSeeking.IMediaSeeking_iface);
}

static inline ParserImpl *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, ParserImpl, filter.IBaseFilter_iface);
}

static inline ParserImpl *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, ParserImpl, filter);
}

IPin *parser_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    ParserImpl *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->pInputPin->pin.IPin_iface;
    else if (index <= filter->cStreams)
        return &filter->sources[index - 1]->pin.pin.IPin_iface;
    return NULL;
}

HRESULT Parser_Create(ParserImpl *pParser, const IBaseFilterVtbl *vtbl, IUnknown *outer,
        const CLSID *clsid, const struct strmbase_filter_ops *func_table, const WCHAR *sink_name,
        PFN_PROCESS_SAMPLE fnProcessSample, PFN_QUERY_ACCEPT fnQueryAccept, PFN_PRE_CONNECT fnPreConnect,
        PFN_CLEANUP fnCleanup, PFN_DISCONNECT fnDisconnect, REQUESTPROC fnRequest,
        STOPPROCESSPROC fnDone, SourceSeeking_ChangeStop stop,
        SourceSeeking_ChangeStart start, SourceSeeking_ChangeRate rate)
{
    HRESULT hr;

    strmbase_filter_init(&pParser->filter, vtbl, outer, clsid, func_table);

    pParser->fnDisconnect = fnDisconnect;

    pParser->cStreams = 0;
    pParser->sources = CoTaskMemAlloc(0 * sizeof(IPin *));

    if (!start)
        start = Parser_ChangeStart;

    if (!stop)
        stop = Parser_ChangeStop;

    if (!rate)
        rate = Parser_ChangeRate;

    SourceSeeking_Init(&pParser->sourceSeeking, &Parser_Seeking_Vtbl, stop, start, rate,  &pParser->filter.csFilter);

    hr = PullPin_Construct(&Parser_InputPin_Vtbl, &pParser->filter, sink_name,
            fnProcessSample, (void *)pParser, fnQueryAccept, fnCleanup, fnRequest,
            fnDone, (IPin **)&pParser->pInputPin);

    if (SUCCEEDED(hr))
    {
        pParser->pInputPin->fnPreConnect = fnPreConnect;
    }
    else
    {
        CoTaskMemFree(pParser->sources);
        strmbase_filter_cleanup(&pParser->filter);
        CoTaskMemFree(pParser);
    }

    return hr;
}

HRESULT WINAPI Parser_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if ( IsEqualIID(riid, &IID_IUnknown)
      || IsEqualIID(riid, &IID_IPersist)
      || IsEqualIID(riid, &IID_IMediaFilter)
      || IsEqualIID(riid, &IID_IBaseFilter) )
        *ppv = &This->filter.IBaseFilter_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

ULONG WINAPI Parser_AddRef(IBaseFilter * iface)
{
    return BaseFilterImpl_AddRef(iface);
}

void Parser_Destroy(ParserImpl *This)
{
    IPin *connected = NULL;
    HRESULT hr;

    PullPin_WaitForStateChange(This->pInputPin, INFINITE);

    /* Don't need to clean up output pins, freeing input pin will do that */
    IPin_ConnectedTo(&This->pInputPin->pin.IPin_iface, &connected);
    if (connected)
    {
        hr = IPin_Disconnect(connected);
        assert(hr == S_OK);
        IPin_Release(connected);
        hr = IPin_Disconnect(&This->pInputPin->pin.IPin_iface);
        assert(hr == S_OK);
    }

    PullPin_destroy(This->pInputPin);

    CoTaskMemFree(This->sources);
    strmbase_filter_cleanup(&This->filter);

    TRACE("Destroying parser\n");
    CoTaskMemFree(This);
}

/** IMediaFilter methods **/

HRESULT WINAPI Parser_Stop(IBaseFilter * iface)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = This->pInputPin;
    ULONG i;

    TRACE("%p->()\n", This);

    EnterCriticalSection(&pin->thread_lock);

    IAsyncReader_BeginFlush(This->pInputPin->pReader);
    EnterCriticalSection(&This->filter.csFilter);

    if (This->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        IAsyncReader_EndFlush(This->pInputPin->pReader);
        LeaveCriticalSection(&pin->thread_lock);
        return S_OK;
    }

    This->filter.state = State_Stopped;

    for (i = 0; i < This->cStreams; ++i)
    {
        BaseOutputPinImpl_Inactive(&This->sources[i]->pin);
    }

    LeaveCriticalSection(&This->filter.csFilter);

    PullPin_PauseProcessing(This->pInputPin);
    PullPin_WaitForStateChange(This->pInputPin, INFINITE);
    IAsyncReader_EndFlush(This->pInputPin->pReader);

    LeaveCriticalSection(&pin->thread_lock);
    return S_OK;
}

HRESULT WINAPI Parser_Pause(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = This->pInputPin;

    TRACE("%p->()\n", This);

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);

    if (This->filter.state == State_Paused)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        LeaveCriticalSection(&pin->thread_lock);
        return S_OK;
    }

    if (This->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        hr = IBaseFilter_Run(iface, -1);
        EnterCriticalSection(&This->filter.csFilter);
    }

    if (SUCCEEDED(hr))
        This->filter.state = State_Paused;

    LeaveCriticalSection(&This->filter.csFilter);
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = This->pInputPin;

    ULONG i;

    TRACE("%p->(%s)\n", This, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);
    {
        HRESULT hr_any = VFW_E_NOT_CONNECTED;

        This->filter.rtStreamStart = tStart;
        if (This->filter.state == State_Running || This->filter.state == State_Paused)
        {
            This->filter.state = State_Running;
            LeaveCriticalSection(&This->filter.csFilter);
            LeaveCriticalSection(&pin->thread_lock);
            return S_OK;
        }

        for (i = 0; i < This->cStreams; ++i)
        {
            hr = BaseOutputPinImpl_Active(&This->sources[i]->pin);
            if (SUCCEEDED(hr))
                hr_any = hr;
        }

        hr = hr_any;
        if (SUCCEEDED(hr))
        {
            LeaveCriticalSection(&This->filter.csFilter);
            hr = PullPin_StartProcessing(This->pInputPin);
            EnterCriticalSection(&This->filter.csFilter);
        }

        if (SUCCEEDED(hr))
            This->filter.state = State_Running;
    }
    LeaveCriticalSection(&This->filter.csFilter);
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = This->pInputPin;
    HRESULT hr = S_OK;

    TRACE("%p->(%d, %p)\n", This, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&pin->thread_lock);
    EnterCriticalSection(&This->filter.csFilter);
    {
        *pState = This->filter.state;
    }
    LeaveCriticalSection(&This->filter.csFilter);

    if (This->pInputPin && (PullPin_WaitForStateChange(This->pInputPin, dwMilliSecsTimeout) == S_FALSE))
        hr = VFW_S_STATE_INTERMEDIATE;
    LeaveCriticalSection(&pin->thread_lock);

    return hr;
}

HRESULT WINAPI Parser_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    ParserImpl *This = impl_from_IBaseFilter(iface);
    PullPin *pin = This->pInputPin;

    TRACE("%p->(%p)\n", This, pClock);

    EnterCriticalSection(&pin->thread_lock);
    BaseFilterImpl_SetSyncSource(iface,pClock);
    LeaveCriticalSection(&pin->thread_lock);

    return S_OK;
}

static const struct strmbase_source_ops source_ops =
{
    {
        Parser_OutputPin_CheckMediaType,
        Parser_OutputPin_GetMediaType
    },
    BaseOutputPinImpl_AttemptConnection,
    Parser_OutputPin_DecideBufferSize,
    Parser_OutputPin_DecideAllocator,
};

HRESULT Parser_AddPin(ParserImpl *filter, const WCHAR *name,
        ALLOCATOR_PROPERTIES *props, const AM_MEDIA_TYPE *mt)
{
    Parser_OutputPin **old_sources;
    Parser_OutputPin *object;

    if (!(object = CoTaskMemAlloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    old_sources = filter->sources;

    filter->sources = CoTaskMemAlloc((filter->cStreams + 1) * sizeof(filter->sources[0]));
    memcpy(filter->sources, old_sources, filter->cStreams * sizeof(filter->sources[0]));
    filter->sources[filter->cStreams] = object;

    strmbase_source_init(&object->pin, &Parser_OutputPin_Vtbl, &filter->filter,
            name, &source_ops);

    object->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    CopyMediaType(object->pmt, mt);
    object->dwSamplesProcessed = 0;
    object->allocProps = *props;
    filter->cStreams++;
    BaseFilterImpl_IncrementPinVersion(&filter->filter);
    CoTaskMemFree(old_sources);

    return S_OK;
}

static void free_source_pin(Parser_OutputPin *pin)
{
    if (pin->pin.pin.pConnectedTo)
    {
        IPin_Disconnect(pin->pin.pin.pConnectedTo);
        IPin_Disconnect(&pin->pin.pin.IPin_iface);
    }

    FreeMediaType(pin->pmt);
    CoTaskMemFree(pin->pmt);
    strmbase_source_cleanup(&pin->pin);
    CoTaskMemFree(pin);
}

static HRESULT Parser_RemoveOutputPins(ParserImpl * This)
{
    /* NOTE: should be in critical section when calling this function */
    ULONG i;
    Parser_OutputPin **old_sources = This->sources;

    TRACE("(%p)\n", This);

    This->sources = CoTaskMemAlloc(0);

    for (i = 0; i < This->cStreams; i++)
        free_source_pin(old_sources[i]);

    BaseFilterImpl_IncrementPinVersion(&This->filter);
    This->cStreams = 0;
    CoTaskMemFree(old_sources);

    return S_OK;
}

static HRESULT WINAPI Parser_ChangeStart(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented start position change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI Parser_ChangeStop(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented stop position change!\n", iface);
    return S_OK;
}

static HRESULT WINAPI Parser_ChangeRate(IMediaSeeking *iface)
{
    FIXME("(%p) filter hasn't implemented rate change!\n", iface);
    return S_OK;
}


static HRESULT WINAPI Parser_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI Parser_Seeking_AddRef(IMediaSeeking * iface)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI Parser_Seeking_Release(IMediaSeeking * iface)
{
    ParserImpl *This = impl_from_IMediaSeeking(iface);

    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static const IMediaSeekingVtbl Parser_Seeking_Vtbl =
{
    Parser_Seeking_QueryInterface,
    Parser_Seeking_AddRef,
    Parser_Seeking_Release,
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

static HRESULT WINAPI Parser_OutputPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    ALLOCATOR_PROPERTIES actual;

    if (ppropInputRequest->cbAlign && ppropInputRequest->cbAlign != This->allocProps.cbAlign)
        FIXME("Requested Buffer cbAlign mismatch %i,%i\n",This->allocProps.cbAlign, ppropInputRequest->cbAlign);
    if (ppropInputRequest->cbPrefix)
        FIXME("Requested Buffer cbPrefix mismatch %i,%i\n",This->allocProps.cbPrefix, ppropInputRequest->cbPrefix);
    if (ppropInputRequest->cbBuffer)
        FIXME("Requested Buffer cbBuffer mismatch %i,%i\n",This->allocProps.cbBuffer, ppropInputRequest->cbBuffer);
    if (ppropInputRequest->cBuffers)
        FIXME("Requested Buffer cBuffers mismatch %i,%i\n",This->allocProps.cBuffers, ppropInputRequest->cBuffers);

    return IMemAllocator_SetProperties(pAlloc, &This->allocProps, &actual);
}

static HRESULT WINAPI Parser_OutputPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    if (iPosition < 0)
        return E_INVALIDARG;
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, This->pmt);
    return S_OK;
}

static HRESULT WINAPI Parser_OutputPin_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    Parser_OutputPin *This = (Parser_OutputPin*)iface;
    HRESULT hr;

    *pAlloc = NULL;

    if (This->alloc)
    {
        hr = IMemInputPin_NotifyAllocator(pPin, This->alloc, This->readonly);
        if (SUCCEEDED(hr))
        {
            *pAlloc = This->alloc;
            IMemAllocator_AddRef(*pAlloc);
        }
    }
    else
        hr = VFW_E_NO_ALLOCATOR;

    return hr;
}

static HRESULT WINAPI Parser_OutputPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    /* The Parser filter does not support querying IMediaSeeking, return it directly */
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &impl_from_IBaseFilter(&This->pin.pin.filter->IBaseFilter_iface)->sourceSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static HRESULT WINAPI Parser_OutputPin_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    Parser_OutputPin *This = unsafe_impl_Parser_OutputPin_from_IPin(iface);
    ParserImpl *parser = impl_from_IBaseFilter(&This->pin.pin.filter->IBaseFilter_iface);

    /* Set the allocator to our input pin's */
    EnterCriticalSection(&parser->filter.csFilter);
    This->alloc = parser->pInputPin->pAlloc;
    LeaveCriticalSection(&parser->filter.csFilter);

    return BaseOutputPinImpl_Connect(iface, pReceivePin, pmt);
}

static HRESULT WINAPI Parser_OutputPin_CheckMediaType(BasePin *pin, const AM_MEDIA_TYPE *pmt)
{
    Parser_OutputPin *This = (Parser_OutputPin *)pin;

    dump_AM_MEDIA_TYPE(pmt);

    return (memcmp(This->pmt, pmt, sizeof(AM_MEDIA_TYPE)) == 0);
}

static const IPinVtbl Parser_OutputPin_Vtbl = 
{
    Parser_OutputPin_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    Parser_OutputPin_Connect,
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

static HRESULT WINAPI Parser_PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    /*
     * It is important to capture the request for the IMediaSeeking interface before it is passed
     * on to PullPin_QueryInterface, this is necessary since the Parser filter does not support
     * querying IMediaSeeking
     */
    if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &impl_from_IBaseFilter(&This->pin.filter->IBaseFilter_iface)->sourceSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return PullPin_QueryInterface(iface, riid, ppv);
}

static HRESULT WINAPI Parser_PullPin_Disconnect(IPin * iface)
{
    HRESULT hr;
    PullPin *This = impl_PullPin_from_IPin(iface);

    TRACE("()\n");

    EnterCriticalSection(&This->thread_lock);
    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (This->pin.pConnectedTo)
        {
            FILTER_STATE state;
            ParserImpl *Parser = impl_from_IBaseFilter(&This->pin.filter->IBaseFilter_iface);

            LeaveCriticalSection(&This->pin.filter->csFilter);
            hr = IBaseFilter_GetState(&This->pin.filter->IBaseFilter_iface, INFINITE, &state);
            EnterCriticalSection(&This->pin.filter->csFilter);

            if (SUCCEEDED(hr) && (state == State_Stopped) && SUCCEEDED(Parser->fnDisconnect(Parser)))
            {
                LeaveCriticalSection(&This->pin.filter->csFilter);
                PullPin_Disconnect(iface);
                EnterCriticalSection(&This->pin.filter->csFilter);
                hr = Parser_RemoveOutputPins(impl_from_IBaseFilter(&This->pin.filter->IBaseFilter_iface));
            }
            else
                hr = VFW_E_NOT_STOPPED;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(&This->pin.filter->csFilter);
    LeaveCriticalSection(&This->thread_lock);

    return hr;
}

static HRESULT WINAPI Parser_PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;

    TRACE("()\n");

    hr = PullPin_ReceiveConnection(iface, pReceivePin, pmt);
    if (FAILED(hr))
    {
        BasePin *This = (BasePin *)iface;

        EnterCriticalSection(&This->filter->csFilter);
        Parser_RemoveOutputPins(impl_from_IBaseFilter(&This->filter->IBaseFilter_iface));
        LeaveCriticalSection(&This->filter->csFilter);
    }

    return hr;
}

static HRESULT WINAPI Parser_PullPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    BasePin *This = (BasePin *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return EnumMediaTypes_Construct(This, BasePinImpl_GetMediaType, BasePinImpl_GetMediaTypeVersion, ppEnum);
}

static const IPinVtbl Parser_InputPin_Vtbl =
{
    Parser_PullPin_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseInputPinImpl_Connect,
    Parser_PullPin_ReceiveConnection,
    Parser_PullPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    PullPin_QueryAccept,
    Parser_PullPin_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    PullPin_EndOfStream,
    PullPin_BeginFlush,
    PullPin_EndFlush,
    PullPin_NewSegment
};
