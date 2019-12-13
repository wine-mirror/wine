/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static const IMemInputPinVtbl MemInputPin_Vtbl;

typedef HRESULT (*SendPinFunc)( IPin *to, LPVOID arg );

static inline struct strmbase_pin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_pin, IPin_iface);
}

/** Helper function, there are a lot of places where the error code is inherited
 * The following rules apply:
 *
 * Return the first received error code (E_NOTIMPL is ignored)
 * If no errors occur: return the first received non-error-code that isn't S_OK
 */
static HRESULT updatehres( HRESULT original, HRESULT new )
{
    if (FAILED( original ) || new == E_NOTIMPL)
        return original;

    if (FAILED( new ) || original == S_OK)
        return new;

    return original;
}

/** Sends a message from a pin further to other, similar pins
 * fnMiddle is called on each pin found further on the stream.
 * fnEnd (can be NULL) is called when the message can't be sent any further (this is a renderer or source)
 *
 * If the pin given is an input pin, the message will be sent downstream to other input pins
 * If the pin given is an output pin, the message will be sent upstream to other output pins
 */
static HRESULT SendFurther(struct strmbase_sink *sink, SendPinFunc func, void *arg)
{
    struct strmbase_pin *pin;
    HRESULT hr = S_OK;
    unsigned int i;

    for (i = 0; (pin = sink->pin.filter->ops->filter_get_pin(sink->pin.filter, i)); ++i)
    {
        if (pin->dir == PINDIR_OUTPUT && pin->peer)
            hr = updatehres(hr, func(pin->peer, arg));
    }

    return hr;
}

static BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards)
{
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) || IsEqualGUID(&pmt2->majortype, &GUID_NULL))) || IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
            ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL)   || IsEqualGUID(&pmt2->subtype, &GUID_NULL)))   || IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

HRESULT strmbase_pin_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT WINAPI pin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    *out = NULL;

    if (pin->pFuncsTable->pin_query_interface
            && SUCCEEDED(hr = pin->pFuncsTable->pin_query_interface(pin, iid, out)))
        return hr;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IPin))
        *out = iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI pin_AddRef(IPin *iface)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    return IBaseFilter_AddRef(&pin->filter->IBaseFilter_iface);
}

static ULONG WINAPI pin_Release(IPin *iface)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    return IBaseFilter_Release(&pin->filter->IBaseFilter_iface);
}

static HRESULT WINAPI pin_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    struct strmbase_pin *This = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ppPin);

    EnterCriticalSection(&This->filter->csFilter);
    {
        if (This->peer)
        {
            *ppPin = This->peer;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
        {
            hr = VFW_E_NOT_CONNECTED;
            *ppPin = NULL;
        }
    }
    LeaveCriticalSection(&This->filter->csFilter);

    return hr;
}

static HRESULT WINAPI pin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    struct strmbase_pin *This = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, pmt);

    EnterCriticalSection(&This->filter->csFilter);
    {
        if (This->peer)
        {
            CopyMediaType(pmt, &This->mt);
            strmbase_dump_media_type(pmt);
            hr = S_OK;
        }
        else
        {
            ZeroMemory(pmt, sizeof(*pmt));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    LeaveCriticalSection(&This->filter->csFilter);

    return hr;
}

static HRESULT WINAPI pin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p, info %p.\n", pin, info);

    info->dir = pin->dir;
    IBaseFilter_AddRef(info->pFilter = &pin->filter->IBaseFilter_iface);
    lstrcpyW(info->achName, pin->name);

    return S_OK;
}

static HRESULT WINAPI pin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p, dir %p.\n", pin, dir);

    *dir = pin->dir;

    return S_OK;
}

static HRESULT WINAPI pin_QueryId(IPin *iface, WCHAR **id)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p, id %p.\n", pin, id);

    if (!(*id = CoTaskMemAlloc((lstrlenW(pin->name) + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    lstrcpyW(*id, pin->name);

    return S_OK;
}

static HRESULT WINAPI pin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    struct strmbase_pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", iface, pmt);
    strmbase_dump_media_type(pmt);

    return (This->pFuncsTable->pin_query_accept(This, pmt) == S_OK ? S_OK : S_FALSE);
}

static HRESULT WINAPI pin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **enum_media_types)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("iface %p, enum_media_types %p.\n", iface, enum_media_types);

    if (FAILED(hr = pin->pFuncsTable->pin_get_media_type(pin, 0, &mt)))
        return hr;
    if (hr == S_OK)
        FreeMediaType(&mt);

    return enum_media_types_create(pin, enum_media_types);
}

static HRESULT WINAPI pin_QueryInternalConnections(IPin *iface, IPin **apPin, ULONG *cPin)
{
    struct strmbase_pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p, %p)\n", This, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

/*** OutputPin implementation ***/

static inline struct strmbase_source *impl_source_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, struct strmbase_source, pin.IPin_iface);
}

static HRESULT WINAPI source_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    struct strmbase_source *This = impl_source_from_IPin(iface);

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    strmbase_dump_media_type(pmt);

    if (!pReceivePin)
        return E_POINTER;

    /* If we try to connect to ourselves, we will definitely deadlock.
     * There are other cases where we could deadlock too, but this
     * catches the obvious case */
    assert(pReceivePin != iface);

    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (This->pin.filter->state != State_Stopped)
        {
            LeaveCriticalSection(&This->pin.filter->csFilter);
            WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
            return VFW_E_NOT_STOPPED;
        }

        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &GUID_NULL))
            hr = This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmt);
        else
        {
            /* negotiate media type */

            IEnumMediaTypes * pEnumCandidates;
            AM_MEDIA_TYPE * pmtCandidate = NULL; /* Candidate media type */

            if (SUCCEEDED(hr = IPin_EnumMediaTypes(iface, &pEnumCandidates)))
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                /* try this filter's media types first */
                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    assert(pmtCandidate);
                    if (!IsEqualGUID(&FORMAT_None, &pmtCandidate->formattype)
                        && !IsEqualGUID(&GUID_NULL, &pmtCandidate->formattype))
                        assert(pmtCandidate->pbFormat);
                    if ((!pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE))
                            && This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmtCandidate) == S_OK)
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                }
                IEnumMediaTypes_Release(pEnumCandidates);
            }

            /* then try receiver filter's media types */
            if (hr != S_OK && SUCCEEDED(hr = IPin_EnumMediaTypes(pReceivePin, &pEnumCandidates))) /* if we haven't already connected successfully */
            {
                ULONG fetched;

                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, &fetched))
                {
                    assert(pmtCandidate);
                    strmbase_dump_media_type(pmtCandidate);
                    if ((!pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE))
                            && This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmtCandidate) == S_OK)
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                } /* while */
                IEnumMediaTypes_Release(pEnumCandidates);
            } /* if not found */
        } /* if negotiate media type */
    } /* if succeeded */
    LeaveCriticalSection(&This->pin.filter->csFilter);

    TRACE(" -- %x\n", hr);
    return hr;
}

static HRESULT WINAPI source_ReceiveConnection(IPin *iface, IPin *pin, const AM_MEDIA_TYPE *pmt)
{
    ERR("(%p)->(%p, %p) incoming connection on an output pin!\n", iface, pin, pmt);
    return E_UNEXPECTED;
}

static HRESULT WINAPI source_Disconnect(IPin *iface)
{
    HRESULT hr;
    struct strmbase_source *This = impl_source_from_IPin(iface);

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (This->pin.filter->state != State_Stopped)
        {
            LeaveCriticalSection(&This->pin.filter->csFilter);
            WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
            return VFW_E_NOT_STOPPED;
        }

        if (This->pMemInputPin)
        {
            IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;
        }
        if (This->pin.peer)
        {
            IPin_Release(This->pin.peer);
            This->pin.peer = NULL;
            FreeMediaType(&This->pin.mt);
            ZeroMemory(&This->pin.mt, sizeof(This->pin.mt));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(&This->pin.filter->csFilter);

    return hr;
}

static HRESULT WINAPI source_EndOfStream(IPin *iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_BeginFlush(IPin *iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_EndFlush(IPin *iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_NewSegment(IPin * iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    TRACE("iface %p, start %s, stop %s, rate %.16e.\n",
            iface, debugstr_time(start), debugstr_time(stop), rate);
    return S_OK;
}

static const IPinVtbl source_vtbl =
{
    pin_QueryInterface,
    pin_AddRef,
    pin_Release,
    source_Connect,
    source_ReceiveConnection,
    source_Disconnect,
    pin_ConnectedTo,
    pin_ConnectionMediaType,
    pin_QueryPinInfo,
    pin_QueryDirection,
    pin_QueryId,
    pin_QueryAccept,
    pin_EnumMediaTypes,
    pin_QueryInternalConnections,
    source_EndOfStream,
    source_BeginFlush,
    source_EndFlush,
    source_NewSegment,
};

HRESULT WINAPI BaseOutputPinImpl_GetDeliveryBuffer(struct strmbase_source *This,
        IMediaSample **ppSample, REFERENCE_TIME *tStart, REFERENCE_TIME *tStop, DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p, %x)\n", This, ppSample, tStart, tStop, dwFlags);

    if (!This->pin.peer)
        hr = VFW_E_NOT_CONNECTED;
    else
    {
        hr = IMemAllocator_GetBuffer(This->pAllocator, ppSample, tStart, tStop, dwFlags);

        if (SUCCEEDED(hr))
            hr = IMediaSample_SetTime(*ppSample, tStart, tStop);
    }

    return hr;
}

/* replaces OutputPin_CommitAllocator */
HRESULT WINAPI BaseOutputPinImpl_Active(struct strmbase_source *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (!This->pin.peer || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IMemAllocator_Commit(This->pAllocator);
    }
    LeaveCriticalSection(&This->pin.filter->csFilter);

    TRACE("--> %08x\n", hr);
    return hr;
}

/* replaces OutputPin_DecommitAllocator */
HRESULT WINAPI BaseOutputPinImpl_Inactive(struct strmbase_source *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (!This->pin.peer || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IMemAllocator_Decommit(This->pAllocator);
    }
    LeaveCriticalSection(&This->pin.filter->csFilter);

    TRACE("--> %08x\n", hr);
    return hr;
}

HRESULT WINAPI BaseOutputPinImpl_InitAllocator(struct strmbase_source *This, IMemAllocator **pMemAlloc)
{
    return CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (LPVOID*)pMemAlloc);
}

HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(struct strmbase_source *This,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    HRESULT hr;

    hr = IMemInputPin_GetAllocator(pPin, pAlloc);

    if (hr == VFW_E_NO_ALLOCATOR)
        /* Input pin provides no allocator, use standard memory allocator */
        hr = BaseOutputPinImpl_InitAllocator(This, pAlloc);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES rProps;
        ZeroMemory(&rProps, sizeof(ALLOCATOR_PROPERTIES));

        IMemInputPin_GetAllocatorRequirements(pPin, &rProps);
        hr = This->pFuncsTable->pfnDecideBufferSize(This, *pAlloc, &rProps);
    }

    if (SUCCEEDED(hr))
        hr = IMemInputPin_NotifyAllocator(pPin, *pAlloc, FALSE);

    return hr;
}

/*** The Construct functions ***/

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(struct strmbase_source *This,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    IMemAllocator * pMemAlloc = NULL;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);

    if ((hr = This->pFuncsTable->base.pin_query_accept(&This->pin, pmt)) != S_OK)
        return hr;

    This->pin.peer = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mt, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &This->pin.IPin_iface, pmt);

    /* get the IMemInputPin interface we will use to deliver samples to the
     * connected pin */
    if (SUCCEEDED(hr))
    {
        This->pMemInputPin = NULL;
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
        {
            hr = This->pFuncsTable->pfnDecideAllocator(This, This->pMemInputPin, &pMemAlloc);
            if (SUCCEEDED(hr))
                This->pAllocator = pMemAlloc;
            else if (pMemAlloc)
                IMemAllocator_Release(pMemAlloc);
        }

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
        {
            if (This->pMemInputPin)
                IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;

            IPin_Disconnect(pReceivePin);
        }
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.peer);
        This->pin.peer = NULL;
        FreeMediaType(&This->pin.mt);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

void strmbase_source_init(struct strmbase_source *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_source_ops *func_table)
{
    memset(pin, 0, sizeof(*pin));
    pin->pin.IPin_iface.lpVtbl = &source_vtbl;
    pin->pin.filter = filter;
    pin->pin.dir = PINDIR_OUTPUT;
    lstrcpyW(pin->pin.name, name);
    pin->pin.pFuncsTable = &func_table->base;
    pin->pFuncsTable = func_table;
}

void strmbase_source_cleanup(struct strmbase_source *pin)
{
    FreeMediaType(&pin->pin.mt);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
}

static struct strmbase_sink *impl_sink_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, pin.IPin_iface);
}

static HRESULT WINAPI sink_Connect(IPin *iface, IPin *pin, const AM_MEDIA_TYPE *pmt)
{
    ERR("(%p)->(%p, %p) outgoing connection on an input pin!\n", iface, pin, pmt);
    return E_UNEXPECTED;
}


static HRESULT WINAPI sink_ReceiveConnection(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    struct strmbase_sink *This = impl_sink_from_IPin(iface);
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    strmbase_dump_media_type(pmt);

    if (!pmt)
        return E_POINTER;

    EnterCriticalSection(&This->pin.filter->csFilter);
    {
        if (This->pin.filter->state != State_Stopped)
        {
            LeaveCriticalSection(&This->pin.filter->csFilter);
            WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
            return VFW_E_NOT_STOPPED;
        }

        if (This->pin.peer)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.pFuncsTable->pin_query_accept(&This->pin, pmt) != S_OK)
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr) && This->pFuncsTable->sink_connect)
            hr = This->pFuncsTable->sink_connect(This, pReceivePin, pmt);

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mt, pmt);
            This->pin.peer = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(&This->pin.filter->csFilter);

    return hr;
}

static HRESULT WINAPI sink_Disconnect(IPin *iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p.\n", pin);

    EnterCriticalSection(&pin->pin.filter->csFilter);

    if (pin->pin.peer)
    {
        if (pin->pFuncsTable->sink_disconnect)
            pin->pFuncsTable->sink_disconnect(pin);

        IPin_Release(pin->pin.peer);
        pin->pin.peer = NULL;
        FreeMediaType(&pin->pin.mt);
        memset(&pin->pin.mt, 0, sizeof(AM_MEDIA_TYPE));
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    LeaveCriticalSection(&pin->pin.filter->csFilter);

    return hr;
}

static HRESULT deliver_endofstream(IPin* pin, LPVOID unused)
{
    return IPin_EndOfStream( pin );
}

static HRESULT WINAPI sink_EndOfStream(IPin *iface)
{
    struct strmbase_sink *This = impl_sink_from_IPin(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->()\n", This);

    if (This->pFuncsTable->sink_eos)
        return This->pFuncsTable->sink_eos(This);

    EnterCriticalSection(&This->pin.filter->csFilter);
    if (This->flushing)
        hr = S_FALSE;
    LeaveCriticalSection(&This->pin.filter->csFilter);

    if (hr == S_OK)
        hr = SendFurther(This, deliver_endofstream, NULL);
    return hr;
}

static HRESULT deliver_beginflush(IPin* pin, LPVOID unused)
{
    return IPin_BeginFlush( pin );
}

static HRESULT WINAPI sink_BeginFlush(IPin *iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p.\n", pin);

    EnterCriticalSection(&pin->pin.filter->csFilter);

    pin->flushing = TRUE;

    if (pin->pFuncsTable->sink_begin_flush)
        hr = pin->pFuncsTable->sink_begin_flush(pin);
    else
        hr = SendFurther(pin, deliver_beginflush, NULL);

    LeaveCriticalSection(&pin->pin.filter->csFilter);

    return hr;
}

static HRESULT deliver_endflush(IPin* pin, LPVOID unused)
{
    return IPin_EndFlush( pin );
}

static HRESULT WINAPI sink_EndFlush(IPin * iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p.\n", pin);

    EnterCriticalSection(&pin->pin.filter->csFilter);

    pin->flushing = FALSE;

    if (pin->pFuncsTable->sink_end_flush)
        hr = pin->pFuncsTable->sink_end_flush(pin);
    else
        hr = SendFurther(pin, deliver_endflush, NULL);

    LeaveCriticalSection(&pin->pin.filter->csFilter);

    return hr;
}

typedef struct newsegmentargs
{
    REFERENCE_TIME tStart, tStop;
    double rate;
} newsegmentargs;

static HRESULT deliver_newsegment(IPin *pin, LPVOID data)
{
    newsegmentargs *args = data;
    return IPin_NewSegment(pin, args->tStart, args->tStop, args->rate);
}

static HRESULT WINAPI sink_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    newsegmentargs args;

    TRACE("iface %p, start %s, stop %s, rate %.16e.\n",
            iface, debugstr_time(start), debugstr_time(stop), rate);

    if (pin->pFuncsTable->sink_new_segment)
        return pin->pFuncsTable->sink_new_segment(pin, start, stop, rate);

    args.tStart = start;
    args.tStop = stop;
    args.rate = rate;

    return SendFurther(pin, deliver_newsegment, &args);
}

static const IPinVtbl sink_vtbl =
{
    pin_QueryInterface,
    pin_AddRef,
    pin_Release,
    sink_Connect,
    sink_ReceiveConnection,
    sink_Disconnect,
    pin_ConnectedTo,
    pin_ConnectionMediaType,
    pin_QueryPinInfo,
    pin_QueryDirection,
    pin_QueryId,
    pin_QueryAccept,
    pin_EnumMediaTypes,
    pin_QueryInternalConnections,
    sink_EndOfStream,
    sink_BeginFlush,
    sink_EndFlush,
    sink_NewSegment,
};

/*** IMemInputPin implementation ***/

static inline struct strmbase_sink *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, IMemInputPin_iface);
}

static HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_QueryInterface(&This->pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_AddRef(&This->pin.IPin_iface);
}

static ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_Release(&This->pin.IPin_iface);
}

static HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppAllocator);

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);

    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

static HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %d)\n", This, iface, pAllocator, bReadOnly);

    if (bReadOnly)
        FIXME("Read only flag not handled yet!\n");

    /* FIXME: Should we release the allocator on disconnection? */
    if (!pAllocator)
    {
        WARN("Null allocator\n");
        return E_POINTER;
    }

    if (This->preferred_allocator && pAllocator != This->preferred_allocator)
        return E_FAIL;

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

static HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pProps);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);
    HRESULT hr = S_FALSE;

    /* this trace commented out for performance reasons */
    /*TRACE("(%p/%p)->(%p)\n", This, iface, pSample);*/
    if (This->pFuncsTable->pfnReceive)
        hr = This->pFuncsTable->pfnReceive(This, pSample);
    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%p, %d, %p)\n", This, iface, pSamples, nSamples, nSamplesProcessed);

    for (*nSamplesProcessed = 0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
    }

    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return S_OK;
}

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

void strmbase_sink_init(struct strmbase_sink *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_sink_ops *func_table, IMemAllocator *allocator)
{
    memset(pin, 0, sizeof(*pin));
    pin->pin.IPin_iface.lpVtbl = &sink_vtbl;
    pin->pin.filter = filter;
    pin->pin.dir = PINDIR_INPUT;
    lstrcpyW(pin->pin.name, name);
    pin->pin.pFuncsTable = &func_table->base;
    pin->pFuncsTable = func_table;
    pin->pAllocator = pin->preferred_allocator = allocator;
    if (pin->preferred_allocator)
        IMemAllocator_AddRef(pin->preferred_allocator);
    pin->IMemInputPin_iface.lpVtbl = &MemInputPin_Vtbl;
}

void strmbase_sink_cleanup(struct strmbase_sink *pin)
{
    FreeMediaType(&pin->pin.mt);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
    pin->pin.IPin_iface.lpVtbl = NULL;
}
