/*              DirectShow Sample Grabber object (QEDIT.DLL)
 *
 * Copyright 2009 Paul Chitescu
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

static WCHAR const vendor_name[] = { 'W', 'i', 'n', 'e', 0 };
static WCHAR const pin_in_name[] = { 'I', 'n', 0 };
static WCHAR const pin_out_name[] = { 'O', 'u', 't', 0 };

/* Sample Grabber pin implementation */
typedef struct _SG_Pin {
    const IPinVtbl* lpVtbl;
    PIN_DIRECTION dir;
    WCHAR const *name;
    struct _SG_Impl *sg;
    IPin *pair;
} SG_Pin;

/* Sample Grabber filter implementation */
typedef struct _SG_Impl {
    const IBaseFilterVtbl* IBaseFilter_Vtbl;
    const ISampleGrabberVtbl* ISampleGrabber_Vtbl;
    const IMemInputPinVtbl* IMemInputPin_Vtbl;
    /* TODO: IMediaPosition, IMediaSeeking, IQualityControl */
    LONG refCount;
    FILTER_INFO info;
    FILTER_STATE state;
    SG_Pin pin_in;
    SG_Pin pin_out;
    IMemAllocator *allocator;
    IReferenceClock *refClock;
    IMemInputPin *memOutput;
    ISampleGrabberCB *grabberIface;
    LONG grabberMethod;
    LONG oneShot;
} SG_Impl;

enum {
    OneShot_None,
    OneShot_Wait,
    OneShot_Past,
};

/* Get the SampleGrabber implementation This pointer from various interface pointers */
static inline SG_Impl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return (SG_Impl *)((char*)iface - FIELD_OFFSET(SG_Impl, IBaseFilter_Vtbl));
}

static inline SG_Impl *impl_from_ISampleGrabber(ISampleGrabber *iface)
{
    return (SG_Impl *)((char*)iface - FIELD_OFFSET(SG_Impl, ISampleGrabber_Vtbl));
}

static inline SG_Impl *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return (SG_Impl *)((char*)iface - FIELD_OFFSET(SG_Impl, IMemInputPin_Vtbl));
}


/* Cleanup at end of life */
static void SampleGrabber_cleanup(SG_Impl *This)
{
    TRACE("(%p)\n", This);
    if (This->info.pGraph)
	WARN("(%p) still joined to filter graph %p\n", This, This->info.pGraph);
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    if (This->refClock)
	IReferenceClock_Release(This->refClock);
    if (This->memOutput)
        IMemInputPin_Release(This->memOutput);
    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
}

/* Common helper AddRef called from all interfaces */
static ULONG SampleGrabber_addref(SG_Impl *This)
{
    ULONG refCount = InterlockedIncrement(&This->refCount);
    TRACE("(%p) new ref = %u\n", This, refCount);
    return refCount;
}

/* Common helper Release called from all interfaces */
static ULONG SampleGrabber_release(SG_Impl *This)
{
    ULONG refCount = InterlockedDecrement(&This->refCount);
    TRACE("(%p) new ref = %u\n", This, refCount);
    if (refCount == 0)
    {
        SampleGrabber_cleanup(This);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

/* Common helper QueryInterface called from all interfaces */
static HRESULT SampleGrabber_query(SG_Impl *This, REFIID riid, void **ppvObject)
{
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter)) {
        SampleGrabber_addref(This);
        *ppvObject = &(This->IBaseFilter_Vtbl);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_ISampleGrabber)) {
        SampleGrabber_addref(This);
        *ppvObject = &(This->ISampleGrabber_Vtbl);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IMemInputPin)) {
        SampleGrabber_addref(This);
        *ppvObject = &(This->IMemInputPin_Vtbl);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        FIXME("IMediaPosition not implemented\n");
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        FIXME("IMediaSeeking not implemented\n");
    else if (IsEqualIID(riid, &IID_IQualityControl))
        FIXME("IQualityControl not implemented\n");
    *ppvObject = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

/* Helper that calls installed sample callbacks */
static void SampleGrabber_callback(SG_Impl *This, IMediaSample *sample)
{
    double time = 0.0;
    REFERENCE_TIME tStart, tEnd;
    if (SUCCEEDED(IMediaSample_GetTime(sample, &tStart, &tEnd)))
        time = 1e-7 * tStart;
    switch (This->grabberMethod) {
        case 0:
	    {
		ULONG ref = IMediaSample_AddRef(sample);
		ISampleGrabberCB_SampleCB(This->grabberIface, time, sample);
		ref = IMediaSample_Release(sample) + 1 - ref;
		if (ref)
		{
		    ERR("(%p) Callback referenced sample %p by %u\n", This, sample, ref);
		    /* ugly as hell but some apps are sooo buggy */
		    while (ref--)
			IMediaSample_Release(sample);
		}
	    }
            break;
        case 1:
            {
                BYTE *data = 0;
                long size = IMediaSample_GetActualDataLength(sample);
                if (size && SUCCEEDED(IMediaSample_GetPointer(sample, &data)) && data)
                    ISampleGrabberCB_BufferCB(This->grabberIface, time, data, size);
            }
            break;
        case -1:
            break;
        default:
            FIXME("unsupported method %ld\n", (long int)This->grabberMethod);
            /* do not bother us again */
            This->grabberMethod = -1;
    }
}


/* SampleGrabber implementation of IBaseFilter interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppvObject)
{
    return SampleGrabber_query(impl_from_IBaseFilter(iface), riid, ppvObject);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_AddRef(IBaseFilter *iface)
{
    return SampleGrabber_addref(impl_from_IBaseFilter(iface));
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_Release(IBaseFilter *iface)
{
    return SampleGrabber_release(impl_from_IBaseFilter(iface));
}

/* IPersist */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    TRACE("(%p)\n", pClassID);
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_SampleGrabber;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Stop(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Stopped;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Pause(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Paused;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Running;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetState(IBaseFilter *iface, DWORD msTout, FILTER_STATE *state)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%u, %p)\n", This, msTout, state);
    if (!state)
        return E_POINTER;
    *state = This->state;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, clock);
    if (clock != This->refClock)
    {
	if (clock)
	    IReferenceClock_AddRef(clock);
	if (This->refClock)
	    IReferenceClock_Release(This->refClock);
	This->refClock = clock;
    }
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **clock)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, clock);
    if (!clock)
        return E_POINTER;
    if (This->refClock)
	IReferenceClock_AddRef(This->refClock);
    *clock = This->refClock;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_EnumPins(IBaseFilter *iface, IEnumPins **pins)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p): stub\n", This, pins);
    if (!pins)
        return E_POINTER;
    return E_OUTOFMEMORY;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **pin)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(id), pin);
    if (!id || !pin)
        return E_POINTER;
    if (!lstrcmpiW(id,pin_in_name))
    {
        SampleGrabber_addref(This);
        *pin = (IPin*)&(This->pin_in.lpVtbl);
        return S_OK;
    }
    else if (!lstrcmpiW(id,pin_out_name))
    {
        SampleGrabber_addref(This);
        *pin = (IPin*)&(This->pin_out.lpVtbl);
        return S_OK;
    }
    *pin = NULL;
    return VFW_E_NOT_FOUND;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *info)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, info);
    if (!info)
        return E_POINTER;
    if (This->info.pGraph)
	IFilterGraph_AddRef(This->info.pGraph);
    *info = This->info;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, LPCWSTR name)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p, %s)\n", This, graph, debugstr_w(name));
    This->info.pGraph = graph;
    if (name)
	lstrcpynW(This->info.achName,name,MAX_FILTER_NAME);
    This->oneShot = OneShot_None;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *vendor)
{
    TRACE("(%p)\n", vendor);
    if (!vendor)
        return E_POINTER;
    *vendor = CoTaskMemAlloc(sizeof(vendor_name));
    CopyMemory(*vendor, vendor_name, sizeof(vendor_name));
    return S_OK;
}


/* SampleGrabber implementation of ISampleGrabber interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_QueryInterface(ISampleGrabber *iface, REFIID riid, void **ppvObject)
{
    return SampleGrabber_query(impl_from_ISampleGrabber(iface), riid, ppvObject);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_AddRef(ISampleGrabber *iface)
{
    return SampleGrabber_addref(impl_from_ISampleGrabber(iface));
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_Release(ISampleGrabber *iface)
{
    return SampleGrabber_release(impl_from_ISampleGrabber(iface));
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetOneShot(ISampleGrabber *iface, BOOL oneShot)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%u)\n", This, oneShot);
    This->oneShot = oneShot ? OneShot_Wait : OneShot_None;
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetMediaType(ISampleGrabber *iface, const AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    if (!type)
        return E_POINTER;
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetConnectedMediaType(ISampleGrabber *iface, AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    if (!type)
        return E_POINTER;
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetBufferSamples(ISampleGrabber *iface, BOOL bufferEm)
{
    TRACE("(%u)\n", bufferEm);
    if (bufferEm) {
        FIXME("buffering not implemented\n");
        return E_NOTIMPL;
    }
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentBuffer(ISampleGrabber *iface, LONG *bufSize, LONG *buffer)
{
    FIXME("(%p, %p): stub\n", bufSize, buffer);
    if (!bufSize)
        return E_POINTER;
    return E_INVALIDARG;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentSample(ISampleGrabber *iface, IMediaSample **sample)
{
    /* MS doesn't implement it either, noone should call it */
    WARN("(%p): not implemented\n", sample);
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetCallback(ISampleGrabber *iface, ISampleGrabberCB *cb, LONG whichMethod)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%p, %u)\n", This, cb, whichMethod);
    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
    This->grabberIface = cb;
    This->grabberMethod = whichMethod;
    if (cb)
        ISampleGrabberCB_AddRef(cb);
    return S_OK;
}


/* SampleGrabber implementation of IMemInputPin interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppvObject)
{
    return SampleGrabber_query(impl_from_IMemInputPin(iface), riid, ppvObject);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IMemInputPin_AddRef(IMemInputPin *iface)
{
    return SampleGrabber_addref(impl_from_IMemInputPin(iface));
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IMemInputPin_Release(IMemInputPin *iface)
{
    return SampleGrabber_release(impl_from_IMemInputPin(iface));
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)->(%p) allocator = %p\n", This, allocator, This->allocator);
    if (!allocator)
        return E_POINTER;
    *allocator = This->allocator;
    if (!*allocator)
        return VFW_E_NO_ALLOCATOR;
    IMemAllocator_AddRef(*allocator);
    return S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_NotifyAllocator(IMemInputPin *iface, IMemAllocator *allocator, BOOL readOnly)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)->(%p, %u) allocator = %p\n", This, allocator, readOnly, This->allocator);
    if (This->allocator == allocator)
        return S_OK;
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    This->allocator = allocator;
    if (allocator)
        IMemAllocator_AddRef(allocator);
    return S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    FIXME("(%p)->(%p): semi-stub\n", This, props);
    if (!props)
        return E_POINTER;
    return This->memOutput ? IMemInputPin_GetAllocatorRequirements(This->memOutput, props) : E_NOTIMPL;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p) output = %p, grabber = %p\n", This, sample, This->memOutput, This->grabberIface);
    if (!sample)
        return E_POINTER;
    if ((This->state != State_Running) || (This->oneShot == OneShot_Past))
        return S_FALSE;
    if (This->grabberIface)
        SampleGrabber_callback(This, sample);
    hr = This->memOutput ? IMemInputPin_Receive(This->memOutput, sample) : S_OK;
    if (This->oneShot == OneShot_Wait) {
        This->oneShot = OneShot_Past;
        hr = S_FALSE;
        if (This->pin_out.pair)
            IPin_EndOfStream(This->pin_out.pair);
    }
    return hr;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **samples, LONG nSamples, LONG *nProcessed)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)->(%p, %u, %p) output = %p, grabber = %p\n", This, samples, nSamples, nProcessed, This->memOutput, This->grabberIface);
    if (!samples || !nProcessed)
        return E_POINTER;
    if ((This->state != State_Running) || (This->oneShot == OneShot_Past))
        return S_FALSE;
    if (This->grabberIface) {
        LONG idx;
        for (idx = 0; idx < nSamples; idx++)
            SampleGrabber_callback(This, samples[idx]);
    }
    return This->memOutput ? IMemInputPin_ReceiveMultiple(This->memOutput, samples, nSamples, nProcessed) : S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)\n", This);
    return This->memOutput ? IMemInputPin_ReceiveCanBlock(This->memOutput) : S_OK;
}


/* SampleGrabber member pin implementation */

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IPin_AddRef(IPin *iface)
{
    return SampleGrabber_addref(((SG_Pin *)iface)->sg);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IPin_Release(IPin *iface)
{
    return SampleGrabber_release(((SG_Pin *)iface)->sg);
}

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IPin_QueryInterface(IPin *iface, REFIID riid, void **ppvObject)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPin)) {
        SampleGrabber_addref(This->sg);
        *ppvObject = This;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IMemInputPin)) {
        SampleGrabber_addref(This->sg);
        *ppvObject = &(This->sg->IMemInputPin_Vtbl);
        return S_OK;
    }
    *ppvObject = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_Connect(IPin *iface, IPin *receiver, const AM_MEDIA_TYPE *mtype)
{
    WARN("(%p, %p): unexpected\n", receiver, mtype);
    return E_UNEXPECTED;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_Connect(IPin *iface, IPin *receiver, const AM_MEDIA_TYPE *type)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p, %p)\n", This, receiver, type);
    if (!receiver)
        return E_POINTER;
    if (This->pair)
        return VFW_E_ALREADY_CONNECTED;
    if (This->sg->state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *type)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p, %p)\n", This, connector, type);
    if (!connector)
        return E_POINTER;
    if (This->pair)
        return VFW_E_ALREADY_CONNECTED;
    if (This->sg->state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *mtype)
{
    WARN("(%p, %p): unexpected\n", connector, mtype);
    return E_UNEXPECTED;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_Disconnect(IPin *iface)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->() pair = %p\n", This, This->pair);
    if (This->sg->state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (This->pair) {
        This->pair = NULL;
        return S_OK;
    }
    return S_FALSE;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_Disconnect(IPin *iface)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->() pair = %p\n", This, This->pair);
    if (This->sg->state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (This->pair) {
        This->pair = NULL;
        if (This->sg->memOutput) {
            IMemInputPin_Release(This->sg->memOutput);
            This->sg->memOutput = NULL;
        }
        return S_OK;
    }
    return S_FALSE;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_ConnectedTo(IPin *iface, IPin **pin)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p) pair = %p\n", This, pin, This->pair);
    if (!pin)
        return E_POINTER;
    *pin = This->pair;
    if (*pin) {
        IPin_AddRef(*pin);
        return S_OK;
    }
    return VFW_E_NOT_CONNECTED;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mtype)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p)\n", This, mtype);
    if (!mtype)
        return E_POINTER;
    return VFW_E_NOT_CONNECTED;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p)\n", This, info);
    if (!info)
        return E_POINTER;
    SampleGrabber_addref(This->sg);
    info->pFilter = (IBaseFilter *)This->sg;
    info->dir = This->dir;
    lstrcpynW(info->achName,This->name,MAX_PIN_NAME);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p)\n", This, dir);
    if (!dir)
        return E_POINTER;
    *dir = This->dir;
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryId(IPin *iface, LPWSTR *id)
{
    SG_Pin *This = (SG_Pin *)iface;
    int len;
    TRACE("(%p)->(%p)\n", This, id);
    if (!id)
        return E_POINTER;
    len = sizeof(WCHAR)*(1+lstrlenW(This->name));
    *id = CoTaskMemAlloc(len);
    CopyMemory(*id, This->name, len);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mtype)
{
    TRACE("(%p)\n", mtype);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **mtypes)
{
    SG_Pin *This = (SG_Pin *)iface;
    FIXME("(%p)->(%p): stub\n", This, mtypes);
    if (!mtypes)
        return E_POINTER;
    return E_OUTOFMEMORY;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *nPins)
{
    SG_Pin *This = (SG_Pin *)iface;
    TRACE("(%p)->(%p, %p) size = %u\n", This, pins, nPins, (nPins ? *nPins : 0));
    if (!nPins)
        return E_POINTER;
    if (*nPins) {
	if (!pins)
	    return E_POINTER;
	IPin_AddRef((IPin*)&This->sg->pin_out.lpVtbl);
	*pins = (IPin*)&This->sg->pin_out.lpVtbl;
	*nPins = 1;
	return S_OK;
    }
    *nPins = 1;
    return S_FALSE;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *nPins)
{
    WARN("(%p, %p): unexpected\n", pins, nPins);
    if (nPins)
        *nPins = 0;
    return E_NOTIMPL;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EndOfStream(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_BeginFlush(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EndFlush(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_NewSegment(IPin *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate)
{
    FIXME(": stub\n");
    return S_OK;
}


/* SampleGrabber vtables and constructor */

static const IBaseFilterVtbl IBaseFilter_VTable =
{
    SampleGrabber_IBaseFilter_QueryInterface,
    SampleGrabber_IBaseFilter_AddRef,
    SampleGrabber_IBaseFilter_Release,
    SampleGrabber_IBaseFilter_GetClassID,
    SampleGrabber_IBaseFilter_Stop,
    SampleGrabber_IBaseFilter_Pause,
    SampleGrabber_IBaseFilter_Run,
    SampleGrabber_IBaseFilter_GetState,
    SampleGrabber_IBaseFilter_SetSyncSource,
    SampleGrabber_IBaseFilter_GetSyncSource,
    SampleGrabber_IBaseFilter_EnumPins,
    SampleGrabber_IBaseFilter_FindPin,
    SampleGrabber_IBaseFilter_QueryFilterInfo,
    SampleGrabber_IBaseFilter_JoinFilterGraph,
    SampleGrabber_IBaseFilter_QueryVendorInfo,
};

static const ISampleGrabberVtbl ISampleGrabber_VTable =
{
    SampleGrabber_ISampleGrabber_QueryInterface,
    SampleGrabber_ISampleGrabber_AddRef,
    SampleGrabber_ISampleGrabber_Release,
    SampleGrabber_ISampleGrabber_SetOneShot,
    SampleGrabber_ISampleGrabber_SetMediaType,
    SampleGrabber_ISampleGrabber_GetConnectedMediaType,
    SampleGrabber_ISampleGrabber_SetBufferSamples,
    SampleGrabber_ISampleGrabber_GetCurrentBuffer,
    SampleGrabber_ISampleGrabber_GetCurrentSample,
    SampleGrabber_ISampleGrabber_SetCallback,
};

static const IMemInputPinVtbl IMemInputPin_VTable =
{
    SampleGrabber_IMemInputPin_QueryInterface,
    SampleGrabber_IMemInputPin_AddRef,
    SampleGrabber_IMemInputPin_Release,
    SampleGrabber_IMemInputPin_GetAllocator,
    SampleGrabber_IMemInputPin_NotifyAllocator,
    SampleGrabber_IMemInputPin_GetAllocatorRequirements,
    SampleGrabber_IMemInputPin_Receive,
    SampleGrabber_IMemInputPin_ReceiveMultiple,
    SampleGrabber_IMemInputPin_ReceiveCanBlock,
};

static const IPinVtbl IPin_In_VTable =
{
    SampleGrabber_IPin_QueryInterface,
    SampleGrabber_IPin_AddRef,
    SampleGrabber_IPin_Release,
    SampleGrabber_In_IPin_Connect,
    SampleGrabber_In_IPin_ReceiveConnection,
    SampleGrabber_In_IPin_Disconnect,
    SampleGrabber_IPin_ConnectedTo,
    SampleGrabber_IPin_ConnectionMediaType,
    SampleGrabber_IPin_QueryPinInfo,
    SampleGrabber_IPin_QueryDirection,
    SampleGrabber_IPin_QueryId,
    SampleGrabber_IPin_QueryAccept,
    SampleGrabber_IPin_EnumMediaTypes,
    SampleGrabber_In_IPin_QueryInternalConnections,
    SampleGrabber_IPin_EndOfStream,
    SampleGrabber_IPin_BeginFlush,
    SampleGrabber_IPin_EndFlush,
    SampleGrabber_IPin_NewSegment,
};

static const IPinVtbl IPin_Out_VTable =
{
    SampleGrabber_IPin_QueryInterface,
    SampleGrabber_IPin_AddRef,
    SampleGrabber_IPin_Release,
    SampleGrabber_Out_IPin_Connect,
    SampleGrabber_Out_IPin_ReceiveConnection,
    SampleGrabber_Out_IPin_Disconnect,
    SampleGrabber_IPin_ConnectedTo,
    SampleGrabber_IPin_ConnectionMediaType,
    SampleGrabber_IPin_QueryPinInfo,
    SampleGrabber_IPin_QueryDirection,
    SampleGrabber_IPin_QueryId,
    SampleGrabber_IPin_QueryAccept,
    SampleGrabber_IPin_EnumMediaTypes,
    SampleGrabber_Out_IPin_QueryInternalConnections,
    SampleGrabber_IPin_EndOfStream,
    SampleGrabber_IPin_BeginFlush,
    SampleGrabber_IPin_EndFlush,
    SampleGrabber_IPin_NewSegment,
};

HRESULT SampleGrabber_create(IUnknown *pUnkOuter, LPVOID *ppv)
{
    SG_Impl* obj = NULL;

    TRACE("(%p,%p)\n", ppv, pUnkOuter);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = CoTaskMemAlloc(sizeof(SG_Impl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(SG_Impl));

    obj->refCount = 1;
    obj->IBaseFilter_Vtbl = &IBaseFilter_VTable;
    obj->ISampleGrabber_Vtbl = &ISampleGrabber_VTable;
    obj->IMemInputPin_Vtbl = &IMemInputPin_VTable;
    obj->pin_in.lpVtbl = &IPin_In_VTable;
    obj->pin_in.dir = PINDIR_INPUT;
    obj->pin_in.name = pin_in_name;
    obj->pin_in.sg = obj;
    obj->pin_in.pair = NULL;
    obj->pin_out.lpVtbl = &IPin_Out_VTable;
    obj->pin_out.dir = PINDIR_OUTPUT;
    obj->pin_out.name = pin_out_name;
    obj->pin_out.sg = obj;
    obj->pin_out.pair = NULL;
    obj->info.achName[0] = 0;
    obj->info.pGraph = NULL;
    obj->state = State_Stopped;
    obj->allocator = NULL;
    obj->refClock = NULL;
    obj->memOutput = NULL;
    obj->grabberIface = NULL;
    obj->grabberMethod = -1;
    obj->oneShot = OneShot_None;
    *ppv = obj;

    return S_OK;
}
