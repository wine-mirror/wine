/*              DirectShow FilterGraph object (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "dshow.h"
#include "wine/debug.h"
#include "quartz_private.h"
#include "ole2.h"
#include "olectl.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "wine/unicode.h"


WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    HWND hWnd;      /* Target window */
    long msg;       /* User window message */
    long instance;  /* User data */
    int  disabled;  /* Disabled messages posting */
} WndNotify;

typedef struct {
    long lEventCode;   /* Event code */
    LONG_PTR lParam1;  /* Param1 */
    LONG_PTR lParam2;  /* Param2 */
} Event;

/* messages ring implementation for queuing events (taken from winmm) */
#define EVENTS_RING_BUFFER_INCREMENT      64
typedef struct {
    Event* messages;
    int ring_buffer_size;
    int msg_tosave;
    int msg_toget;
    CRITICAL_SECTION msg_crst;
    HANDLE msg_event; /* Signaled for no empty queue */
} EventsQueue;

static int EventsQueue_Init(EventsQueue* omr)
{
    omr->msg_toget = 0;
    omr->msg_tosave = 0;
    omr->msg_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    omr->ring_buffer_size = EVENTS_RING_BUFFER_INCREMENT;
    omr->messages = CoTaskMemAlloc(omr->ring_buffer_size * sizeof(Event));
    ZeroMemory(omr->messages, omr->ring_buffer_size * sizeof(Event));

    InitializeCriticalSection(&omr->msg_crst);
    omr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": EventsQueue.msg_crst");
    return TRUE;
}

static int EventsQueue_Destroy(EventsQueue* omr)
{
    CloseHandle(omr->msg_event);
    CoTaskMemFree(omr->messages);
    omr->msg_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&omr->msg_crst);
    return TRUE;
}

static int EventsQueue_PutEvent(EventsQueue* omr, Event* evt)
{
    EnterCriticalSection(&omr->msg_crst);
    if ((omr->msg_toget == ((omr->msg_tosave + 1) % omr->ring_buffer_size)))
    {
	int old_ring_buffer_size = omr->ring_buffer_size;
	omr->ring_buffer_size += EVENTS_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = HeapReAlloc(GetProcessHeap(),0,omr->messages, omr->ring_buffer_size * sizeof(Event));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between omr->msg_tosave and
	   omr->msg_toget.
	*/
	if (omr->msg_tosave < omr->msg_toget)
	{
	    memmove(&(omr->messages[omr->msg_toget + EVENTS_RING_BUFFER_INCREMENT]),
		    &(omr->messages[omr->msg_toget]),
		    sizeof(Event)*(old_ring_buffer_size - omr->msg_toget)
		    );
	    omr->msg_toget += EVENTS_RING_BUFFER_INCREMENT;
	}
    }
    omr->messages[omr->msg_tosave] = *evt;
    SetEvent(omr->msg_event);
    omr->msg_tosave = (omr->msg_tosave + 1) % omr->ring_buffer_size;
    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

static int EventsQueue_GetEvent(EventsQueue* omr, Event* evt, long msTimeOut)
{
    if (WaitForSingleObject(omr->msg_event, msTimeOut) != WAIT_OBJECT_0)
	return FALSE;
	
    EnterCriticalSection(&omr->msg_crst);

    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&omr->msg_crst);
	return FALSE;
    }

    *evt = omr->messages[omr->msg_toget];
    omr->msg_toget = (omr->msg_toget + 1) % omr->ring_buffer_size;

    /* Mark the buffer as empty if needed */
    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
	ResetEvent(omr->msg_event);

    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

#define MAX_ITF_CACHE_ENTRIES 3
typedef struct _ITF_CACHE_ENTRY {
   const IID* riid;
   IBaseFilter* filter;
   IUnknown* iface;
} ITF_CACHE_ENTRY;

typedef struct _IFilterGraphImpl {
    const IGraphBuilderVtbl *IGraphBuilder_vtbl;
    const IMediaControlVtbl *IMediaControl_vtbl;
    const IMediaSeekingVtbl *IMediaSeeking_vtbl;
    const IBasicAudioVtbl *IBasicAudio_vtbl;
    const IBasicVideoVtbl *IBasicVideo_vtbl;
    const IVideoWindowVtbl *IVideoWindow_vtbl;
    const IMediaEventExVtbl *IMediaEventEx_vtbl;
    const IMediaFilterVtbl *IMediaFilter_vtbl;
    const IMediaEventSinkVtbl *IMediaEventSink_vtbl;
    const IGraphConfigVtbl *IGraphConfig_vtbl;
    const IMediaPositionVtbl *IMediaPosition_vtbl;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IBasicVideo2 */
    /* IFilterChain */
    /* IFilterGraph2 */
    /* IFilterMapper2 */
    /* IGraphVersion */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceMananger */
    /* IServiceProvider */
    /* IVideoFrameStep */

    LONG ref;
    IFilterMapper2 * pFilterMapper2;
    IBaseFilter ** ppFiltersInGraph;
    LPWSTR * pFilterNames;
    int nFilters;
    int filterCapacity;
    long nameIndex;
    IReferenceClock *refClock;
    EventsQueue evqueue;
    HANDLE hEventCompletion;
    int CompletionStatus;
    WndNotify notif;
    int nRenderers;
    int EcCompleteCount;
    int HandleEcComplete;
    int HandleEcRepaint;
    int HandleEcClockChanged;
    OAFilterState state;
    CRITICAL_SECTION cs;
    ITF_CACHE_ENTRY ItfCacheEntries[MAX_ITF_CACHE_ENTRIES];
    int nItfCacheEntries;
} IFilterGraphImpl;


static HRESULT Filtergraph_QueryInterface(IFilterGraphImpl *This,
					  REFIID riid,
					  LPVOID *ppvObj) {
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);
    
    if (IsEqualGUID(&IID_IUnknown, riid) ||
	IsEqualGUID(&IID_IFilterGraph, riid) ||
	IsEqualGUID(&IID_IGraphBuilder, riid)) {
        *ppvObj = &(This->IGraphBuilder_vtbl);
        TRACE("   returning IGraphBuilder interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaControl, riid)) {
        *ppvObj = &(This->IMediaControl_vtbl);
        TRACE("   returning IMediaControl interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &(This->IMediaSeeking_vtbl);
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicAudio, riid)) {
        *ppvObj = &(This->IBasicAudio_vtbl);
        TRACE("   returning IBasicAudio interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicVideo, riid)) {
        *ppvObj = &(This->IBasicVideo_vtbl);
        TRACE("   returning IBasicVideo interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IVideoWindow, riid)) {
        *ppvObj = &(This->IVideoWindow_vtbl);
        TRACE("   returning IVideoWindow interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEvent, riid) ||
	   IsEqualGUID(&IID_IMediaEventEx, riid)) {
        *ppvObj = &(This->IMediaEventEx_vtbl);
        TRACE("   returning IMediaEvent(Ex) interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaFilter, riid) ||
          IsEqualGUID(&IID_IPersist, riid)) {
        *ppvObj = &(This->IMediaFilter_vtbl);
        TRACE("   returning IMediaFilter interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEventSink, riid)) {
        *ppvObj = &(This->IMediaEventSink_vtbl);
        TRACE("   returning IMediaEventSink interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IGraphConfig, riid)) {
        *ppvObj = &(This->IGraphConfig_vtbl);
        TRACE("   returning IGraphConfig interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaPosition, riid)) {
        *ppvObj = &(This->IMediaPosition_vtbl);
        TRACE("   returning IMediaPosition interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
	FIXME("unknown interface %s\n", debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    InterlockedIncrement(&This->ref);
    return S_OK;
}

static ULONG Filtergraph_AddRef(IFilterGraphImpl *This) {
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);
    
    return ref;
}

static ULONG Filtergraph_Release(IFilterGraphImpl *This) {
    ULONG ref = InterlockedDecrement(&This->ref);
    
    TRACE("(%p)->(): new ref = %d\n", This, ref);
    
    if (ref == 0) {
        int i;

        if (This->refClock)
            IReferenceClock_Release(This->refClock);

        for (i = 0; i < This->nFilters; i++)
        {
            IBaseFilter_SetSyncSource(This->ppFiltersInGraph[i], NULL);
            IBaseFilter_Release(This->ppFiltersInGraph[i]);
        }
        for (i = 0; i < This->nItfCacheEntries; i++)
        {
            if (This->ItfCacheEntries[i].iface)
                IUnknown_Release(This->ItfCacheEntries[i].iface);
        }
	IFilterMapper2_Release(This->pFilterMapper2);
	CloseHandle(This->hEventCompletion);
	EventsQueue_Destroy(&This->evqueue);
        This->cs.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&This->cs);
	CoTaskMemFree(This->ppFiltersInGraph);
	CoTaskMemFree(This->pFilterNames);
	CoTaskMemFree(This);
    }
    return ref;
}


/*** IUnknown methods ***/
static HRESULT WINAPI GraphBuilder_QueryInterface(IGraphBuilder *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);
    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI GraphBuilder_AddRef(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph AddRef\n", This, iface);
    
    return Filtergraph_AddRef(This);
}

static ULONG WINAPI GraphBuilder_Release(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph Release\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IFilterGraph methods ***/
static HRESULT WINAPI GraphBuilder_AddFilter(IGraphBuilder *iface,
					     IBaseFilter *pFilter,
					     LPCWSTR pName) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    HRESULT hr;
    int i,j;
    WCHAR* wszFilterName = NULL;
    int duplicate_name = FALSE;
    
    TRACE("(%p/%p)->(%p, %s (%p))\n", This, iface, pFilter, debugstr_w(pName), pName);

    wszFilterName = (WCHAR*) CoTaskMemAlloc( (pName ? strlenW(pName) + 6 : 5) * sizeof(WCHAR) );
    
    if (pName)
    {
	/* Check if name already exists */
        for(i = 0; i < This->nFilters; i++)
	    if (!strcmpW(This->pFilterNames[i], pName))
	    {
		duplicate_name = TRUE;
		break;
	    }
    }

    /* If no name given or name already existing, generate one */
    if (!pName || duplicate_name)
    {
	static const WCHAR wszFmt1[] = {'%','s',' ','%','0','4','d',0};
	static const WCHAR wszFmt2[] = {'%','0','4','d',0};

	for (j = 0; j < 10000 ; j++)
	{
	    /* Create name */
	    if (pName)
		sprintfW(wszFilterName, wszFmt1, pName, This->nameIndex);
	    else
		sprintfW(wszFilterName, wszFmt2, This->nameIndex);
	    TRACE("Generated name %s\n", debugstr_w(wszFilterName));

	    /* Check if the generated name already exists */
	    for(i = 0; i < This->nFilters; i++)
	    	if (!strcmpW(This->pFilterNames[i], wszFilterName))
		    break;

	    /* Compute next index and exit if generated name is suitable */
	    if (This->nameIndex++ == 10000)
		This->nameIndex = 1;
	    if (i == This->nFilters)
		break;
	}
	/* Unable to find a suitable name */
	if (j == 10000)
	{
	    CoTaskMemFree(wszFilterName);
	    return VFW_E_DUPLICATE_NAME;
	}
    }
    else
	memcpy(wszFilterName, pName, (strlenW(pName) + 1) * sizeof(WCHAR));

    if (This->nFilters + 1 > This->filterCapacity)
    {
        int newCapacity = This->filterCapacity ? 2 * This->filterCapacity : 1;
        IBaseFilter ** ppNewFilters = CoTaskMemAlloc(newCapacity * sizeof(IBaseFilter*));
        LPWSTR * pNewNames = CoTaskMemAlloc(newCapacity * sizeof(LPWSTR));
        memcpy(ppNewFilters, This->ppFiltersInGraph, This->nFilters * sizeof(IBaseFilter*));
        memcpy(pNewNames, This->pFilterNames, This->nFilters * sizeof(LPWSTR));
        if (!This->filterCapacity)
        {
            CoTaskMemFree(This->ppFiltersInGraph);
            CoTaskMemFree(This->pFilterNames);
        }
        This->ppFiltersInGraph = ppNewFilters;
        This->pFilterNames = pNewNames;
        This->filterCapacity = newCapacity;
    }

    hr = IBaseFilter_JoinFilterGraph(pFilter, (IFilterGraph *)This, wszFilterName);

    if (SUCCEEDED(hr))
    {
        IBaseFilter_AddRef(pFilter);
        This->ppFiltersInGraph[This->nFilters] = pFilter;
        This->pFilterNames[This->nFilters] = wszFilterName;
        This->nFilters++;
        IBaseFilter_SetSyncSource(pFilter, This->refClock);
    }
    else
	CoTaskMemFree(wszFilterName);

    if (SUCCEEDED(hr) && duplicate_name)
	return VFW_S_DUPLICATE_NAME;
	
    return hr;
}

static HRESULT WINAPI GraphBuilder_RemoveFilter(IGraphBuilder *iface,
						IBaseFilter *pFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    int i;
    HRESULT hr = E_FAIL;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFilter);

    /* FIXME: check graph is stopped */

    for (i = 0; i < This->nFilters; i++)
    {
        if (This->ppFiltersInGraph[i] == pFilter)
        {
            IEnumPins *penumpins;
            hr = IBaseFilter_EnumPins(pFilter, &penumpins);
            if (SUCCEEDED(hr)) {
                IPin *ppin;
                while(IEnumPins_Next(penumpins, 1, &ppin, NULL) == S_OK) {
                    IPin_Disconnect(ppin);
                    IPin_Release(ppin);
                }
                IEnumPins_Release(penumpins);
            }

            hr = IBaseFilter_JoinFilterGraph(pFilter, NULL, This->pFilterNames[i]);
            if (SUCCEEDED(hr))
            {
                IBaseFilter_SetSyncSource(pFilter, NULL);
                IBaseFilter_Release(pFilter);
                CoTaskMemFree(This->pFilterNames[i]);
                memmove(This->ppFiltersInGraph+i, This->ppFiltersInGraph+i+1, sizeof(IBaseFilter*)*(This->nFilters - 1 - i));
                memmove(This->pFilterNames+i, This->pFilterNames+i+1, sizeof(LPWSTR)*(This->nFilters - 1 - i));
                This->nFilters--;
                /* Invalidate interfaces in the cache */
                for (i = 0; i < This->nItfCacheEntries; i++)
                    if (pFilter == This->ItfCacheEntries[i].filter)
                    {
                        IUnknown_Release(This->ItfCacheEntries[i].iface);
                        This->ItfCacheEntries[i].iface = NULL;
                        This->ItfCacheEntries[i].filter = NULL;
                    }
                return S_OK;
            }
            break;
        }
    }

    return hr; /* FIXME: check this error code */
}

static HRESULT WINAPI GraphBuilder_EnumFilters(IGraphBuilder *iface,
					      IEnumFilters **ppEnum) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumFiltersImpl_Construct(This->ppFiltersInGraph, This->nFilters, ppEnum);
}

static HRESULT WINAPI GraphBuilder_FindFilterByName(IGraphBuilder *iface,
						    LPCWSTR pName,
						    IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    int i;

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_w(pName), pName, ppFilter);

    *ppFilter = NULL;

    for (i = 0; i < This->nFilters; i++)
    {
        if (!strcmpW(pName, This->pFilterNames[i]))
        {
            *ppFilter = This->ppFiltersInGraph[i];
            IBaseFilter_AddRef(*ppFilter);
            return S_OK;
        }
    }

    return E_FAIL; /* FIXME: check this error code */
}

/* NOTE: despite the implication, it doesn't matter which
 * way round you put in the input and output pins */
static HRESULT WINAPI GraphBuilder_ConnectDirect(IGraphBuilder *iface,
						 IPin *ppinIn,
						 IPin *ppinOut,
						 const AM_MEDIA_TYPE *pmt) {
    PIN_DIRECTION dir;
    HRESULT hr;

    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);

    /* FIXME: check pins are in graph */

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning first pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning second pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    hr = IPin_QueryDirection(ppinIn, &dir);
    if (SUCCEEDED(hr))
    {
        if (dir == PINDIR_INPUT)
            hr = IPin_Connect(ppinOut, ppinIn, pmt);
        else
            hr = IPin_Connect(ppinIn, ppinOut, pmt);
    }

    return hr;
}

static HRESULT WINAPI GraphBuilder_Reconnect(IGraphBuilder *iface,
					     IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    IPin *pConnectedTo = NULL;
    HRESULT hr;
    PIN_DIRECTION pindir;

    IPin_QueryDirection(ppin, &pindir);
    hr = IPin_ConnectedTo(ppin, &pConnectedTo);
    if (FAILED(hr)) {
        TRACE("Querying connected to failed: %x\n", hr);
        return hr; 
    }
    IPin_Disconnect(ppin);
    IPin_Disconnect(pConnectedTo);
    if (pindir == PINDIR_INPUT)
        hr = IPin_Connect(pConnectedTo, ppin, NULL);
    else
        hr = IPin_Connect(ppin, pConnectedTo, NULL);
    IPin_Release(pConnectedTo);
    if (FAILED(hr))
        ERR("Reconnecting pins failed, pins are not connected now..\n");
    TRACE("(%p->%p) -- %p %p -> %x\n", iface, This, ppin, pConnectedTo, hr);
    return hr;
}

static HRESULT WINAPI GraphBuilder_Disconnect(IGraphBuilder *iface,
					      IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppin);

    return IPin_Disconnect(ppin);
}

static HRESULT WINAPI GraphBuilder_SetDefaultSyncSource(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", iface, This);

    return S_OK;
}

static HRESULT GetFilterInfo(IMoniker* pMoniker, GUID* pclsid, VARIANT* pvar)
{
    static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    IPropertyBag * pPropBagCat = NULL;
    HRESULT hr;

    VariantInit(pvar);
    V_VT(pvar) = VT_BSTR;

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszClsidName, pvar, NULL);

    if (SUCCEEDED(hr))
        hr = CLSIDFromString(V_UNION(pvar, bstrVal), pclsid);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszFriendlyName, pvar, NULL);

    if (SUCCEEDED(hr))
        TRACE("Moniker = %s - %s\n", debugstr_guid(pclsid), debugstr_w(V_UNION(pvar, bstrVal)));

    if (pPropBagCat)
        IPropertyBag_Release(pPropBagCat);

    return hr;
}

static HRESULT GetInternalConnections(IBaseFilter* pfilter, IPin* pinputpin, IPin*** pppins, ULONG* pnb)
{
    HRESULT hr;
    ULONG nb = 0;

    TRACE("(%p, %p, %p, %p)\n", pfilter, pinputpin, pppins, pnb);
    hr = IPin_QueryInternalConnections(pinputpin, NULL, &nb);
    if (hr == S_OK) {
        /* Rendered input */
    } else if (hr == S_FALSE) {
        *pppins = CoTaskMemAlloc(sizeof(IPin*)*nb);
        hr = IPin_QueryInternalConnections(pinputpin, *pppins, &nb);
        if (hr != S_OK) {
            ERR("Error (%x)\n", hr);
        }
    } else if (hr == E_NOTIMPL) {
        /* Input connected to all outputs */
        IEnumPins* penumpins;
        IPin* ppin;
        int i = 0;
        TRACE("E_NOTIMPL\n");
        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            ERR("filter Enumpins failed (%x)\n", hr);
            return hr;
        }
        i = 0;
        /* Count output pins */
        while(IEnumPins_Next(penumpins, 1, &ppin, &nb) == S_OK) {
            PIN_DIRECTION pindir;
            IPin_QueryDirection(ppin, &pindir);
            if (pindir == PINDIR_OUTPUT)
                i++;
            IPin_Release(ppin);
        }
        *pppins = CoTaskMemAlloc(sizeof(IPin*)*i);
        /* Retrieve output pins */
        IEnumPins_Reset(penumpins);
        i = 0;
        while(IEnumPins_Next(penumpins, 1, &ppin, &nb) == S_OK) {
            PIN_DIRECTION pindir;
            IPin_QueryDirection(ppin, &pindir);
            if (pindir == PINDIR_OUTPUT)
                (*pppins)[i++] = ppin;
            else
                IPin_Release(ppin);
        }
        IEnumPins_Release(penumpins);
        nb = i;
        if (FAILED(hr)) {
            ERR("Next failed (%x)\n", hr);
            return hr;
        }
    } else if (FAILED(hr)) {
        ERR("Cannot get internal connection (%x)\n", hr);
        return hr;
    }

    *pnb = nb;
    return S_OK;
}

/*** IGraphBuilder methods ***/
static HRESULT WINAPI GraphBuilder_Connect(IGraphBuilder *iface,
					   IPin *ppinOut,
					   IPin *ppinIn) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    HRESULT hr;
    AM_MEDIA_TYPE* mt;
    IEnumMediaTypes* penummt;
    ULONG nbmt;
    IEnumPins* penumpins;
    IEnumMoniker* pEnumMoniker;
    GUID tab[2];
    ULONG nb;
    IMoniker* pMoniker;
    ULONG pin;
    PIN_INFO PinInfo;
    CLSID FilterCLSID;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, ppinOut, ppinIn);

    if (TRACE_ON(quartz))
    {
	hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning first pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning second pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    /* Try direct connection first */
    hr = IPin_Connect(ppinOut, ppinIn, NULL);
    if (SUCCEEDED(hr)) {
        return S_OK;
    }
    TRACE("Direct connection failed, trying to insert other filters\n");

    hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
    if (FAILED(hr))
       return hr;

    hr = IBaseFilter_GetClassID(PinInfo.pFilter, &FilterCLSID);
    IBaseFilter_Release(PinInfo.pFilter);
    if (FAILED(hr))
       return hr;

    /* Find the appropriate transform filter than can transform the minor media type of output pin of the upstream 
     * filter to the minor mediatype of input pin of the renderer */
    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        ERR("EnumMediaTypes (%x)\n", hr);
        return hr;
    }

    hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
    if (FAILED(hr)) {
        ERR("IEnumMediaTypes_Next (%x)\n", hr);
        return hr;
    }

    if (!nbmt) {
        ERR("No media type found!\n");
        return S_OK;
    }
    TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

    /* Try to find a suitable filter that can connect to the pin to render */
    tab[0] = mt->majortype;
    tab[1] = mt->subtype;
    hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, 0, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
    if (FAILED(hr)) {
        ERR("Unable to enum filters (%x)\n", hr);
        return hr;
    }
    
    while(IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
    {
        VARIANT var;
        GUID clsid;
        IPin** ppins;
        IPin* ppinfilter = NULL;
        IBaseFilter* pfilter = NULL;

        hr = GetFilterInfo(pMoniker, &clsid, &var);
        IMoniker_Release(pMoniker);
        if (FAILED(hr)) {
            ERR("Unable to retrieve filter info (%x)\n", hr);
            goto error;
        }

        if (IsEqualGUID(&clsid, &FilterCLSID)) {
            /* Skip filter (same as the one the output pin belongs to) */
            goto error;
        }

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
        if (FAILED(hr)) {
            ERR("Unable to create filter (%x), trying next one\n", hr);
            goto error;
        }

        hr = IGraphBuilder_AddFilter(iface, pfilter, V_UNION(&var, bstrVal));
        if (FAILED(hr)) {
            ERR("Unable to add filter (%x)\n", hr);
            IBaseFilter_Release(pfilter);
            pfilter = NULL;
            goto error;
        }

        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            ERR("Enumpins (%x)\n", hr);
            goto error;
        }

        hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin);
        IEnumPins_Release(penumpins);

        if (FAILED(hr)) {
            ERR("Next (%x)\n", hr);
            goto error;
        }
        if (pin == 0) {
            ERR("No Pin\n");
            goto error;
        }

        hr = IPin_Connect(ppinOut, ppinfilter, NULL);
        if (FAILED(hr)) {
            TRACE("Cannot connect to filter (%x), trying next one\n", hr);
            goto error;
        }
        TRACE("Successfully connected to filter, follow chain...\n");

        /* Render all output pins of the filter by calling IGraphBuilder_Render on each of them */
        hr = GetInternalConnections(pfilter, ppinfilter, &ppins, &nb);

        if (SUCCEEDED(hr)) {
            int i;
            if (nb == 0) {
                IPin_Disconnect(ppinOut);
                goto error;
            }
            TRACE("pins to consider: %d\n", nb);
            for(i = 0; i < nb; i++) {
                TRACE("Processing pin %d\n", i);
                hr = IGraphBuilder_Connect(iface, ppins[i], ppinIn);
                if (FAILED(hr)) {
                   TRACE("Cannot render pin %p (%x)\n", ppinfilter, hr);
                }
                IPin_Release(ppins[i]);
                if (SUCCEEDED(hr)) break;
            }
            while (++i < nb) IPin_Release(ppins[i]);
            CoTaskMemFree(ppins);
            IPin_Release(ppinfilter);
            IBaseFilter_Release(pfilter);
            break;
        }

error:
        if (ppinfilter) IPin_Release(ppinfilter);
        if (pfilter) {
            IGraphBuilder_RemoveFilter(iface, pfilter);
            IBaseFilter_Release(pfilter);
        }
    }

    IEnumMediaTypes_Release(penummt);
    DeleteMediaType(mt);
    
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Render(IGraphBuilder *iface,
					  IPin *ppinOut) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    IEnumMediaTypes* penummt;
    AM_MEDIA_TYPE* mt;
    ULONG nbmt;
    HRESULT hr;

    IEnumMoniker* pEnumMoniker;
    GUID tab[2];
    ULONG nb;
    IMoniker* pMoniker;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppinOut);

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        ERR("EnumMediaTypes (%x)\n", hr);
        return hr;
    }

    while(1)
    {
        hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
        if (FAILED(hr)) {
            ERR("IEnumMediaTypes_Next (%x)\n", hr);
            return hr;
        }
        if (!nbmt)
            break;
        TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
        TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

        /* Try to find a suitable renderer with the same media type */
        tab[0] = mt->majortype;
        tab[1] = GUID_NULL;
        hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, 0, TRUE, 1, tab, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL);
        if (FAILED(hr)) {
            ERR("Unable to enum filters (%x)\n", hr);
            return hr;
        }

        while(IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
        {
            VARIANT var;
            GUID clsid;
            IPin* ppinfilter;
            IBaseFilter* pfilter = NULL;
            IEnumPins* penumpins;
            ULONG pin;

            hr = GetFilterInfo(pMoniker, &clsid, &var);
            IMoniker_Release(pMoniker);
            if (FAILED(hr)) {
                ERR("Unable to retrieve filter info (%x)\n", hr);
                goto error;
            }

            hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
            if (FAILED(hr)) {
               ERR("Unable to create filter (%x), trying next one\n", hr);
               goto error;
            }

            hr = IGraphBuilder_AddFilter(iface, pfilter, V_UNION(&var, bstrVal));
            if (FAILED(hr)) {
                ERR("Unable to add filter (%x)\n", hr);
                IBaseFilter_Release(pfilter);
                pfilter = NULL;
                goto error;
            }

            hr = IBaseFilter_EnumPins(pfilter, &penumpins);
            if (FAILED(hr)) {
                ERR("Splitter Enumpins (%x)\n", hr);
                goto error;
            }
            hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin);
            IEnumPins_Release(penumpins);
            if (FAILED(hr)) {
               ERR("Next (%x)\n", hr);
               goto error;
            }
            if (pin == 0) {
               ERR("No Pin\n");
               goto error;
            }

	    /* Connect the pin to render to the renderer */
            hr = IGraphBuilder_Connect(iface, ppinOut, ppinfilter);
            if (FAILED(hr)) {
                TRACE("Unable to connect to renderer (%x)\n", hr);
                IPin_Release(ppinfilter);
                goto error;
            }
            IPin_Release(ppinfilter);
            IBaseFilter_Release(pfilter);
            pfilter = NULL;
            break;

error:
            if (pfilter) {
                IGraphBuilder_RemoveFilter(iface, pfilter);
                IBaseFilter_Release(pfilter);
            }
	}
       
        DeleteMediaType(mt);
        break;	
    }

    IEnumMediaTypes_Release(penummt);
    
    return S_OK;
}

static HRESULT WINAPI GraphBuilder_RenderFile(IGraphBuilder *iface,
					      LPCWSTR lpcwstrFile,
					      LPCWSTR lpcwstrPlayList) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    static const WCHAR string[] = {'R','e','a','d','e','r',0};
    IBaseFilter* preader = NULL;
    IBaseFilter* psplitter = NULL;
    IPin* ppinreader = NULL;
    IPin* ppinsplitter;
    IEnumPins* penumpins;
    ULONG pin;
    HRESULT hr;
    IEnumMoniker* pEnumMoniker = NULL;
    GUID tab[2];
    IPin** ppins = NULL;
    ULONG nb;
    IMoniker* pMoniker;
    IFileSourceFilter* pfile = NULL;
    AM_MEDIA_TYPE mt;
    WCHAR* filename;

    TRACE("(%p/%p)->(%s, %s)\n", This, iface, debugstr_w(lpcwstrFile), debugstr_w(lpcwstrPlayList));

    if (lpcwstrPlayList != NULL)
        return E_INVALIDARG;

    hr = IGraphBuilder_AddSourceFilter(iface, lpcwstrFile, string, &preader);

    /* Retrieve file media type */
    if (SUCCEEDED(hr))
        hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter, (LPVOID*)&pfile);
    if (SUCCEEDED(hr)) {
        hr = IFileSourceFilter_GetCurFile(pfile, &filename, &mt);
        IFileSourceFilter_Release(pfile);
    }

    if (SUCCEEDED(hr))
        hr = IBaseFilter_EnumPins(preader, &penumpins);
    if (SUCCEEDED(hr)) {
        hr = IEnumPins_Next(penumpins, 1, &ppinreader, &pin);
        IEnumPins_Release(penumpins);
    }

    if (SUCCEEDED(hr)) {
        tab[0] = mt.majortype;
        tab[1] = mt.subtype;
        hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, 0, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
    }

    if (FAILED(hr))
    {
        if (ppinreader)
            IPin_Release(ppinreader);
        if (pEnumMoniker)
            IEnumMoniker_Release(pEnumMoniker);
        if (preader) {
             IGraphBuilder_RemoveFilter(iface, preader);
             IBaseFilter_Release(preader);
        }
        return hr;
    }

    hr = VFW_E_CANNOT_RENDER;
    while(IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
    {
        VARIANT var;
        GUID clsid;

        hr = GetFilterInfo(pMoniker, &clsid, &var);
        IMoniker_Release(pMoniker);
        if (FAILED(hr)) {
            ERR("Unable to retrieve filter info (%x)\n", hr);
            continue;
        }

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&psplitter);
        if (FAILED(hr)) {
           ERR("Unable to create filter (%x), trying next one\n", hr);
           continue;
        }

        hr = IGraphBuilder_AddFilter(iface, psplitter, V_UNION(&var, bstrVal));
        if (FAILED(hr)) {
            ERR("Unable add filter (%x)\n", hr);
            IBaseFilter_Release(psplitter);
            continue;
        }

        /* Connect file source and splitter filters together */
        /* Make the splitter analyze incoming data */

        hr = IBaseFilter_EnumPins(psplitter, &penumpins);
        if (SUCCEEDED(hr)) {
            hr = IEnumPins_Next(penumpins, 1, &ppinsplitter, &pin);
            IEnumPins_Release(penumpins);
        }

        if (SUCCEEDED(hr))
            hr = IPin_Connect(ppinreader, ppinsplitter, NULL);

        /* Make sure there's some output pins in the filter */
        if (SUCCEEDED(hr))
            hr = GetInternalConnections(psplitter, ppinsplitter, &ppins, &nb);
        if (SUCCEEDED(hr)) {
            if(nb == 0) {
                IPin_Disconnect(ppinreader);
                TRACE("No output pins found in filter\n");
                hr = VFW_E_CANNOT_RENDER;
            }
        }

        if (ppinsplitter)
            IPin_Release(ppinsplitter);
        ppinsplitter = NULL;

        if (SUCCEEDED(hr)) {
            TRACE("Successfully connected to filter\n");
            break;
        }

        TRACE("Cannot connect to filter (%x), trying next one\n", hr);

        if (ppins) {
            CoTaskMemFree(ppins);
            ppins = NULL;
        }
        IGraphBuilder_RemoveFilter(iface, psplitter);
        IBaseFilter_Release(psplitter);
        psplitter = NULL;
    }

    /* Render all output pin of the splitter by calling IGraphBuilder_Render on each of them */
    if (SUCCEEDED(hr)) {
        int partial = 0;
        int i;
        TRACE("pins to consider: %d\n", nb);
        for(i = 0; i < nb; i++) {
            TRACE("Processing pin %d\n", i);
            hr = IGraphBuilder_Render(iface, ppins[i]);
            if (FAILED(hr)) {
                ERR("Cannot render pin %p (%x)\n", ppins[i], hr);
                partial = 1;
            }
            IPin_Release(ppins[i]);
        }
        CoTaskMemFree(ppins);

        hr = (partial ? VFW_S_PARTIAL_RENDER : S_OK);
    }

    IPin_Release(ppinreader);
    IBaseFilter_Release(preader);
    if (psplitter)
        IBaseFilter_Release(psplitter);

    return hr;
}

static HRESULT WINAPI GraphBuilder_AddSourceFilter(IGraphBuilder *iface,
						   LPCWSTR lpcwstrFileName,
						   LPCWSTR lpcwstrFilterName,
						   IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    HRESULT hr;
    IBaseFilter* preader;
    IFileSourceFilter* pfile = NULL;
    AM_MEDIA_TYPE mt;
    WCHAR* filename;

    TRACE("(%p/%p)->(%s, %s, %p)\n", This, iface, debugstr_w(lpcwstrFileName), debugstr_w(lpcwstrFilterName), ppFilter);

    /* Instantiate a file source filter */ 
    hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&preader);
    if (FAILED(hr)) {
        ERR("Unable to create file source filter (%x)\n", hr);
        return hr;
    }

    hr = IGraphBuilder_AddFilter(iface, preader, lpcwstrFilterName);
    if (FAILED(hr)) {
        ERR("Unable add filter (%x)\n", hr);
        IBaseFilter_Release(preader);
        return hr;
    }

    hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter, (LPVOID*)&pfile);
    if (FAILED(hr)) {
        ERR("Unable to get IFileSourceInterface (%x)\n", hr);
        goto error;
    }

    /* Load the file in the file source filter */
    hr = IFileSourceFilter_Load(pfile, lpcwstrFileName, NULL);
    if (FAILED(hr)) {
        ERR("Load (%x)\n", hr);
        goto error;
    }
    
    IFileSourceFilter_GetCurFile(pfile, &filename, &mt);
    if (FAILED(hr)) {
        ERR("GetCurFile (%x)\n", hr);
        goto error;
    }
    TRACE("File %s\n", debugstr_w(filename));
    TRACE("MajorType %s\n", debugstr_guid(&mt.majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt.subtype));

    if (ppFilter)
        *ppFilter = preader;
    IFileSourceFilter_Release(pfile);

    return S_OK;
    
error:
    if (pfile)
        IFileSourceFilter_Release(pfile);
    IGraphBuilder_RemoveFilter(iface, preader);
    IBaseFilter_Release(preader);
       
    return hr;
}

static HRESULT WINAPI GraphBuilder_SetLogFile(IGraphBuilder *iface,
					      DWORD_PTR hFile) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%08x): stub !!!\n", This, iface, (DWORD) hFile);

    return S_OK;
}

static HRESULT WINAPI GraphBuilder_Abort(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI GraphBuilder_ShouldOperationContinue(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IGraphBuilderVtbl IGraphBuilder_VTable =
{
    GraphBuilder_QueryInterface,
    GraphBuilder_AddRef,
    GraphBuilder_Release,
    GraphBuilder_AddFilter,
    GraphBuilder_RemoveFilter,
    GraphBuilder_EnumFilters,
    GraphBuilder_FindFilterByName,
    GraphBuilder_ConnectDirect,
    GraphBuilder_Reconnect,
    GraphBuilder_Disconnect,
    GraphBuilder_SetDefaultSyncSource,
    GraphBuilder_Connect,
    GraphBuilder_Render,
    GraphBuilder_RenderFile,
    GraphBuilder_AddSourceFilter,
    GraphBuilder_SetLogFile,
    GraphBuilder_Abort,
    GraphBuilder_ShouldOperationContinue
};

/*** IUnknown methods ***/
static HRESULT WINAPI MediaControl_QueryInterface(IMediaControl *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaControl_AddRef(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaControl_Release(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);

}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaControl_GetTypeInfoCount(IMediaControl *iface,
						    UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetTypeInfo(IMediaControl *iface,
					       UINT iTInfo,
					       LCID lcid,
					       ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetIDsOfNames(IMediaControl *iface,
						 REFIID riid,
						 LPOLESTR*rgszNames,
						 UINT cNames,
						 LCID lcid,
						 DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaControl_Invoke(IMediaControl *iface,
					  DISPID dispIdMember,
					  REFIID riid,
					  LCID lcid,
					  WORD wFlags,
					  DISPPARAMS*pDispParams,
					  VARIANT*pVarResult,
					  EXCEPINFO*pExepInfo,
					  UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

typedef HRESULT(WINAPI *fnFoundFilter)(IBaseFilter *);

static HRESULT ExploreGraph(IFilterGraphImpl* pGraph, IPin* pOutputPin, fnFoundFilter FoundFilter)
{
    HRESULT hr;
    IPin* pInputPin;
    IPin** ppPins;
    ULONG nb;
    ULONG i;
    PIN_INFO PinInfo;

    TRACE("%p %p\n", pGraph, pOutputPin);
    PinInfo.pFilter = NULL;

    hr = IPin_ConnectedTo(pOutputPin, &pInputPin);

    if (SUCCEEDED(hr))
    {
        hr = IPin_QueryPinInfo(pInputPin, &PinInfo);
        if (SUCCEEDED(hr))
            hr = GetInternalConnections(PinInfo.pFilter, pInputPin, &ppPins, &nb);
        IPin_Release(pInputPin);
    }

    if (SUCCEEDED(hr))
    {
        if (nb == 0)
        {
            TRACE("Reached a renderer\n");
            /* Count renderers for end of stream notification */
            pGraph->nRenderers++;
        }
        else
        {
            for(i = 0; i < nb; i++)
            {
                /* Explore the graph downstream from this pin
		 * FIXME: We should prevent exploring from a pin more than once. This can happens when
		 * several input pins are connected to the same output (a MUX for instance). */
                ExploreGraph(pGraph, ppPins[i], FoundFilter);
                IPin_Release(ppPins[i]);
            }

            CoTaskMemFree(ppPins);
        }
        TRACE("Doing stuff with filter %p\n", PinInfo.pFilter);
        FoundFilter(PinInfo.pFilter);
    }

    if (PinInfo.pFilter) IBaseFilter_Release(PinInfo.pFilter);
    return hr;
}

static HRESULT WINAPI SendRun(IBaseFilter *pFilter) {
   return IBaseFilter_Run(pFilter, 0);
}

static HRESULT WINAPI SendPause(IBaseFilter *pFilter) {
   return IBaseFilter_Pause(pFilter);
}

static HRESULT WINAPI SendStop(IBaseFilter *pFilter) {
   return IBaseFilter_Stop(pFilter);
}

static HRESULT SendFilterMessage(IMediaControl *iface, fnFoundFilter FoundFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    int i;
    IBaseFilter* pfilter;
    IEnumPins* pEnum;
    HRESULT hr;
    IPin* pPin;
    DWORD dummy;
    PIN_DIRECTION dir;
    TRACE("(%p/%p)->()\n", This, iface);

    /* Explorer the graph from source filters to renderers, determine renderers
     * number and run filters from renderers to source filters */
    This->nRenderers = 0;  
    ResetEvent(This->hEventCompletion);

    for(i = 0; i < This->nFilters; i++)
    {
        BOOL source = TRUE;
        pfilter = This->ppFiltersInGraph[i];
        hr = IBaseFilter_EnumPins(pfilter, &pEnum);
        if (hr != S_OK)
        {
            ERR("Enum pins failed %x\n", hr);
            continue;
        }
        /* Check if it is a source filter */
        while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
        {
            IPin_QueryDirection(pPin, &dir);
            IPin_Release(pPin);
            if (dir == PINDIR_INPUT)
            {
                source = FALSE;
                break;
            }
        }
        if (source)
        {
            TRACE("Found a source filter %p\n", pfilter);
            IEnumPins_Reset(pEnum);
            while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
            {
                /* Explore the graph downstream from this pin */
                ExploreGraph(This, pPin, FoundFilter);
                IPin_Release(pPin);
            }
            FoundFilter(pfilter);
        }
        IEnumPins_Release(pEnum);
    }

    return S_FALSE;
}

/*** IMediaControl methods ***/
static HRESULT WINAPI MediaControl_Run(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Running) return S_OK;

    EnterCriticalSection(&This->cs);
    SendFilterMessage(iface, SendRun);
    This->state = State_Running;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaControl_Pause(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Paused) return S_OK;

    EnterCriticalSection(&This->cs);
    SendFilterMessage(iface, SendPause);
    This->state = State_Paused;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaControl_Stop(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Stopped) return S_OK;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Running) SendFilterMessage(iface, SendPause);
    SendFilterMessage(iface, SendStop);
    This->state = State_Stopped;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaControl_GetState(IMediaControl *iface,
					    LONG msTimeout,
					    OAFilterState *pfs) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %p): semi-stub !!!\n", This, iface, msTimeout, pfs);

    EnterCriticalSection(&This->cs);

    *pfs = This->state;

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI MediaControl_RenderFile(IMediaControl *iface,
					      BSTR strFilename) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p)): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename);

    return S_OK;
}

static HRESULT WINAPI MediaControl_AddSourceFilter(IMediaControl *iface,
						   BSTR strFilename,
						   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_FilterCollection(IMediaControl *iface,
							IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_RegFilterCollection(IMediaControl *iface,
							   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_StopWhenReady(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IMediaControlVtbl IMediaControl_VTable =
{
    MediaControl_QueryInterface,
    MediaControl_AddRef,
    MediaControl_Release,
    MediaControl_GetTypeInfoCount,
    MediaControl_GetTypeInfo,
    MediaControl_GetIDsOfNames,
    MediaControl_Invoke,
    MediaControl_Run,
    MediaControl_Pause,
    MediaControl_Stop,
    MediaControl_GetState,
    MediaControl_RenderFile,
    MediaControl_AddSourceFilter,
    MediaControl_get_FilterCollection,
    MediaControl_get_RegFilterCollection,
    MediaControl_StopWhenReady
};


/*** IUnknown methods ***/
static HRESULT WINAPI MediaSeeking_QueryInterface(IMediaSeeking *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IMediaSeeking methods ***/
static HRESULT WINAPI MediaSeeking_GetCapabilities(IMediaSeeking *iface,
						   DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCapabilities);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(IMediaSeeking *iface,
						     DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCapabilities);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(IMediaSeeking *iface,
							GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(IMediaSeeking *iface,
						 GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(IMediaSeeking *iface,
						 const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetDuration(IMediaSeeking *iface,
					       LONGLONG *pDuration) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDuration);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(IMediaSeeking *iface,
						   LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pStop);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(IMediaSeeking *iface,
						      LONGLONG *pCurrent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCurrent);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface,
						     LONGLONG *pTarget,
						     const GUID *pTargetFormat,
						     LONGLONG Source,
						     const GUID *pSourceFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, 0x%s, %p): stub !!!\n", This, iface, pTarget,
        pTargetFormat, wine_dbgstr_longlong(Source), pSourceFormat);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						DWORD dwCurrentFlags,
						LONGLONG *pStop,
						DWORD dwStopFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %08x, %p, %08x): stub !!!\n", This, iface, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pCurrent, pStop);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface,
						LONGLONG *pEarliest,
						LONGLONG *pLatest) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pEarliest, pLatest);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface,
					   double dRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%f): stub !!!\n", This, iface, dRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface,
					   double *pdRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pdRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface,
					      LONGLONG *pllPreroll) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pllPreroll);

    return S_OK;
}


static const IMediaSeekingVtbl IMediaSeeking_VTable =
{
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

/*** IUnknown methods ***/
static HRESULT WINAPI MediaPosition_QueryInterface(IMediaPosition* iface, REFIID riid, void** ppvObj){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaPosition_AddRef(IMediaPosition *iface){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaPosition_Release(IMediaPosition *iface){
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaPosition_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaPosition_GetTypeInfoCount(IMediaPosition *iface, UINT* pctinfo){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetTypeInfo(IMediaPosition *iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetIDsOfNames(IMediaPosition* iface, REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_Invoke(IMediaPosition* iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr){
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

/*** IMediaPosition methods ***/
static HRESULT WINAPI MediaPosition_get_Duration(IMediaPosition * iface, REFTIME *plength){
    FIXME("(%p)->(%p) stub!\n", iface, plength);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_CurrentPosition(IMediaPosition * iface, REFTIME llTime){
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_CurrentPosition(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_StopTime(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_StopTime(IMediaPosition * iface, REFTIME llTime){
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_PrerollTime(IMediaPosition * iface, REFTIME *pllTime){
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_PrerollTime(IMediaPosition * iface, REFTIME llTime){
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_Rate(IMediaPosition * iface, double dRate){
    FIXME("(%p)->(%f) stub!\n", iface, dRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_get_Rate(IMediaPosition * iface, double *pdRate){
    FIXME("(%p)->(%p) stub!\n", iface, pdRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekForward(IMediaPosition * iface, LONG *pCanSeekForward){
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekForward);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekBackward(IMediaPosition * iface, LONG *pCanSeekBackward){
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekBackward);
    return E_NOTIMPL;
}


static const IMediaPositionVtbl IMediaPosition_VTable =
{
    MediaPosition_QueryInterface,
    MediaPosition_AddRef,
    MediaPosition_Release,
    MediaPosition_GetTypeInfoCount,
    MediaPosition_GetTypeInfo,
    MediaPosition_GetIDsOfNames,
    MediaPosition_Invoke,
    MediaPosition_get_Duration,
    MediaPosition_put_CurrentPosition,
    MediaPosition_get_CurrentPosition,
    MediaPosition_get_StopTime,
    MediaPosition_put_StopTime,
    MediaPosition_get_PrerollTime,
    MediaPosition_put_PrerollTime,
    MediaPosition_put_Rate,
    MediaPosition_get_Rate,
    MediaPosition_CanSeekForward,
    MediaPosition_CanSeekBackward
};

static HRESULT GetTargetInterface(IFilterGraphImpl* pGraph, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    int i;
    int entry;

    /* Check if the interface type is already registered */
    for (entry = 0; entry < pGraph->nItfCacheEntries; entry++)
        if (riid == pGraph->ItfCacheEntries[entry].riid)
        {
            if (pGraph->ItfCacheEntries[entry].iface)
            {
                /* Return the interface if available */
                *ppvObj = pGraph->ItfCacheEntries[entry].iface;
                return S_OK;
            }
            break;
        }

    if (entry >= MAX_ITF_CACHE_ENTRIES)
    {
        FIXME("Not enough space to store interface in the cache\n");
        return E_OUTOFMEMORY;
    }

    /* Find a filter supporting the requested interface */
    for (i = 0; i < pGraph->nFilters; i++)
    {
        hr = IBaseFilter_QueryInterface(pGraph->ppFiltersInGraph[i], riid, ppvObj);
        if (hr == S_OK)
        {
            pGraph->ItfCacheEntries[entry].riid = riid;
            pGraph->ItfCacheEntries[entry].filter = pGraph->ppFiltersInGraph[i];
            pGraph->ItfCacheEntries[entry].iface = (IUnknown*)*ppvObj;
            if (entry >= pGraph->nItfCacheEntries)
                pGraph->nItfCacheEntries++;
            return S_OK;
        }
        if (hr != E_NOINTERFACE)
            return hr;
    }

    return hr;
}

/*** IUnknown methods ***/
static HRESULT WINAPI BasicAudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI BasicAudio_AddRef(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI BasicAudio_Release(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI BasicAudio_GetTypeInfoCount(IBasicAudio *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetTypeInfoCount(pBasicAudio, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_GetTypeInfo(IBasicAudio *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetTypeInfo(pBasicAudio, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_GetIDsOfNames(IBasicAudio *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_GetIDsOfNames(pBasicAudio, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_Invoke(IBasicAudio *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_Invoke(pBasicAudio, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI BasicAudio_put_Volume(IBasicAudio *iface,
					    long lVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, lVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Volume(pBasicAudio, lVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Volume(IBasicAudio *iface,
					    long *plVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Volume(pBasicAudio, plVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_put_Balance(IBasicAudio *iface,
					     long lBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, lBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Balance(pBasicAudio, lBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Balance(IBasicAudio *iface,
					     long *plBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Balance(pBasicAudio, plBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicAudioVtbl IBasicAudio_VTable =
{
    BasicAudio_QueryInterface,
    BasicAudio_AddRef,
    BasicAudio_Release,
    BasicAudio_GetTypeInfoCount,
    BasicAudio_GetTypeInfo,
    BasicAudio_GetIDsOfNames,
    BasicAudio_Invoke,
    BasicAudio_put_Volume,
    BasicAudio_get_Volume,
    BasicAudio_put_Balance,
    BasicAudio_get_Balance
};

/*** IUnknown methods ***/
static HRESULT WINAPI BasicVideo_QueryInterface(IBasicVideo *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI BasicVideo_AddRef(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI BasicVideo_Release(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI BasicVideo_GetTypeInfoCount(IBasicVideo *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetTypeInfoCount(pBasicVideo, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetTypeInfo(IBasicVideo *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetTypeInfo(pBasicVideo, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetIDsOfNames(IBasicVideo *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetIDsOfNames(pBasicVideo, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_Invoke(IBasicVideo *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_Invoke(pBasicVideo, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI BasicVideo_get_AvgTimePerFrame(IBasicVideo *iface,
						     REFTIME *pAvgTimePerFrame) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_AvgTimePerFrame(pBasicVideo, pAvgTimePerFrame);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitRate(IBasicVideo *iface,
					     long *pBitRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitRate(pBasicVideo, pBitRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitErrorRate(IBasicVideo *iface,
						  long *pBitErrorRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitErrorRate(pBasicVideo, pBitErrorRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoWidth(IBasicVideo *iface,
						long *pVideoWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoWidth(pBasicVideo, pVideoWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoHeight(IBasicVideo *iface,
						 long *pVideoHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoHeight(pBasicVideo, pVideoHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceLeft(IBasicVideo *iface,
						long SourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceLeft(pBasicVideo, SourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceLeft(IBasicVideo *iface,
						long *pSourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceLeft(pBasicVideo, pSourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceWidth(IBasicVideo *iface,
						 long SourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceWidth(pBasicVideo, SourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceWidth(IBasicVideo *iface,
						 long *pSourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceWidth(pBasicVideo, pSourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceTop(IBasicVideo *iface,
					       long SourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceTop(pBasicVideo, SourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceTop(IBasicVideo *iface,
					       long *pSourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceTop(pBasicVideo, pSourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceHeight(IBasicVideo *iface,
						  long SourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceHeight(pBasicVideo, SourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceHeight(IBasicVideo *iface,
						  long *pSourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceHeight(pBasicVideo, pSourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationLeft(IBasicVideo *iface,
						     long DestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationLeft(pBasicVideo, DestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationLeft(IBasicVideo *iface,
						     long *pDestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationLeft(pBasicVideo, pDestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationWidth(IBasicVideo *iface,
						      long DestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationWidth(pBasicVideo, DestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationWidth(IBasicVideo *iface,
						      long *pDestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationWidth(pBasicVideo, pDestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationTop(IBasicVideo *iface,
						    long DestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationTop(pBasicVideo, DestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationTop(IBasicVideo *iface,
						    long *pDestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationTop(pBasicVideo, pDestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationHeight(IBasicVideo *iface,
						       long DestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationHeight(pBasicVideo, DestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationHeight(IBasicVideo *iface,
						       long *pDestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationHeight(pBasicVideo, pDestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetSourcePosition(IBasicVideo *iface,
						   long Left,
						   long Top,
						   long Width,
						   long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetSourcePosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetSourcePosition(IBasicVideo *iface,
						   long *pLeft,
						   long *pTop,
						   long *pWidth,
						   long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetSourcePosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultSourcePosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultSourcePosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDestinationPosition(IBasicVideo *iface,
							long Left,
							long Top,
							long Width,
							long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDestinationPosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetDestinationPosition(IBasicVideo *iface,
							long *pLeft,
							long *pTop,
							long *pWidth,
							long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetDestinationPosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultDestinationPosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultDestinationPosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoSize(IBasicVideo *iface,
					      long *pWidth,
					      long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoSize(pBasicVideo, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoPaletteEntries(IBasicVideo *iface,
							long StartIndex,
							long Entries,
							long *pRetrieved,
							long *pPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld, %ld, %p, %p)\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoPaletteEntries(pBasicVideo, StartIndex, Entries, pRetrieved, pPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetCurrentImage(IBasicVideo *iface,
						 long *pBufferSize,
						 long *pDIBImage) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pBufferSize, pDIBImage);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetCurrentImage(pBasicVideo, pBufferSize, pDIBImage);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultSource(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultSource(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultDestination(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);
    IBasicVideo* pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultDestination(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}


static const IBasicVideoVtbl IBasicVideo_VTable =
{
    BasicVideo_QueryInterface,
    BasicVideo_AddRef,
    BasicVideo_Release,
    BasicVideo_GetTypeInfoCount,
    BasicVideo_GetTypeInfo,
    BasicVideo_GetIDsOfNames,
    BasicVideo_Invoke,
    BasicVideo_get_AvgTimePerFrame,
    BasicVideo_get_BitRate,
    BasicVideo_get_BitErrorRate,
    BasicVideo_get_VideoWidth,
    BasicVideo_get_VideoHeight,
    BasicVideo_put_SourceLeft,
    BasicVideo_get_SourceLeft,
    BasicVideo_put_SourceWidth,
    BasicVideo_get_SourceWidth,
    BasicVideo_put_SourceTop,
    BasicVideo_get_SourceTop,
    BasicVideo_put_SourceHeight,
    BasicVideo_get_SourceHeight,
    BasicVideo_put_DestinationLeft,
    BasicVideo_get_DestinationLeft,
    BasicVideo_put_DestinationWidth,
    BasicVideo_get_DestinationWidth,
    BasicVideo_put_DestinationTop,
    BasicVideo_get_DestinationTop,
    BasicVideo_put_DestinationHeight,
    BasicVideo_get_DestinationHeight,
    BasicVideo_SetSourcePosition,
    BasicVideo_GetSourcePosition,
    BasicVideo_SetDefaultSourcePosition,
    BasicVideo_SetDestinationPosition,
    BasicVideo_GetDestinationPosition,
    BasicVideo_SetDefaultDestinationPosition,
    BasicVideo_GetVideoSize,
    BasicVideo_GetVideoPaletteEntries,
    BasicVideo_GetCurrentImage,
    BasicVideo_IsUsingDefaultSource,
    BasicVideo_IsUsingDefaultDestination
};


/*** IUnknown methods ***/
static HRESULT WINAPI VideoWindow_QueryInterface(IVideoWindow *iface,
						 REFIID riid,
						 LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI VideoWindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI VideoWindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI VideoWindow_GetTypeInfoCount(IVideoWindow *iface,
						   UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pctinfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetTypeInfoCount(pVideoWindow, pctinfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetTypeInfo(IVideoWindow *iface,
					      UINT iTInfo,
					      LCID lcid,
					      ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p)\n", This, iface, iTInfo, lcid, ppTInfo);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetTypeInfo(pVideoWindow, iTInfo, lcid, ppTInfo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetIDsOfNames(IVideoWindow *iface,
						REFIID riid,
						LPOLESTR*rgszNames,
						UINT cNames,
						LCID lcid,
						DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p)\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetIDsOfNames(pVideoWindow, riid, rgszNames, cNames, lcid, rgDispId);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_Invoke(IVideoWindow *iface,
					 DISPID dispIdMember,
					 REFIID riid,
					 LCID lcid,
					 WORD wFlags,
					 DISPPARAMS*pDispParams,
					 VARIANT*pVarResult,
					 EXCEPINFO*pExepInfo,
					 UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p)\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_Invoke(pVideoWindow, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    LeaveCriticalSection(&This->cs);

    return hr;
}


/*** IVideoWindow methods ***/
static HRESULT WINAPI VideoWindow_put_Caption(IVideoWindow *iface,
					      BSTR strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;
    
    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyle(IVideoWindow *iface,
						  long WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyle(IVideoWindow *iface,
						  long *WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyleEx(IVideoWindow *iface,
						    long WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyleEx(IVideoWindow *iface,
						    long *WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_AutoShow(IVideoWindow *iface,
					       long AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_AutoShow(IVideoWindow *iface,
					       long *AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowState(IVideoWindow *iface,
						  long WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowState(IVideoWindow *iface,
						  long *WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BackgroundPalette(IVideoWindow *iface,
							long BackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, BackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BackgroundPalette(pVideoWindow, BackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BackgroundPalette(IVideoWindow *iface,
							long *pBackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BackgroundPalette(pVideoWindow, pBackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Visible(IVideoWindow *iface,
					      long Visible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Visible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Visible(pVideoWindow, Visible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Visible(IVideoWindow *iface,
					      long *pVisible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Visible(pVideoWindow, pVisible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Left(IVideoWindow *iface,
					   long Left) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Left);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Left(pVideoWindow, Left);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Left(IVideoWindow *iface,
					   long *pLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Left(pVideoWindow, pLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Width(IVideoWindow *iface,
					    long Width) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Width);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Width(pVideoWindow, Width);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Width(IVideoWindow *iface,
					    long *pWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Width(pVideoWindow, pWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Top(IVideoWindow *iface,
					  long Top) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Top);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Top(pVideoWindow, Top);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Top(IVideoWindow *iface,
					  long *pTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Top(pVideoWindow, pTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Height(IVideoWindow *iface,
					     long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Height(pVideoWindow, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Height(IVideoWindow *iface,
					     long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Height(pVideoWindow, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_MessageDrain(IVideoWindow *iface,
						   OAHWND Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_MessageDrain(IVideoWindow *iface,
						   OAHWND *Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BorderColor(IVideoWindow *iface,
						  long *Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BorderColor(IVideoWindow *iface,
						  long Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface,
						     long *FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface,
						     long FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowForeground(IVideoWindow *iface,
						      long Focus) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Focus);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowForeground(pVideoWindow, Focus);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
						     long uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x, %ld, %08lx, %08lx)\n", This, iface, (DWORD) hwnd, uMsg, wParam, lParam);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_NotifyOwnerMessage(pVideoWindow, hwnd, uMsg, wParam, lParam);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowPosition(IVideoWindow *iface,
						    long Left,
						    long Top,
						    long Width,
						    long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;
    
    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowPosition(pVideoWindow, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetWindowPosition(IVideoWindow *iface,
						    long *pLeft,
						    long *pTop,
						    long *pWidth,
						    long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetWindowPosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMinIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMinIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMaxIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMaxIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetRestorePosition(IVideoWindow *iface,
						     long *pLeft,
						     long *pTop,
						     long *pWidth,
						     long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetRestorePosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_HideCursor(IVideoWindow *iface,
					     long HideCursor) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, HideCursor);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_HideCursor(pVideoWindow, HideCursor);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_IsCursorHidden(IVideoWindow *iface,
						 long *CursorHidden) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    IVideoWindow* pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, CursorHidden);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_IsCursorHidden(pVideoWindow, CursorHidden);

    LeaveCriticalSection(&This->cs);

    return hr;
}


static const IVideoWindowVtbl IVideoWindow_VTable =
{
    VideoWindow_QueryInterface,
    VideoWindow_AddRef,
    VideoWindow_Release,
    VideoWindow_GetTypeInfoCount,
    VideoWindow_GetTypeInfo,
    VideoWindow_GetIDsOfNames,
    VideoWindow_Invoke,
    VideoWindow_put_Caption,
    VideoWindow_get_Caption,
    VideoWindow_put_WindowStyle,
    VideoWindow_get_WindowStyle,
    VideoWindow_put_WindowStyleEx,
    VideoWindow_get_WindowStyleEx,
    VideoWindow_put_AutoShow,
    VideoWindow_get_AutoShow,
    VideoWindow_put_WindowState,
    VideoWindow_get_WindowState,
    VideoWindow_put_BackgroundPalette,
    VideoWindow_get_BackgroundPalette,
    VideoWindow_put_Visible,
    VideoWindow_get_Visible,
    VideoWindow_put_Left,
    VideoWindow_get_Left,
    VideoWindow_put_Width,
    VideoWindow_get_Width,
    VideoWindow_put_Top,
    VideoWindow_get_Top,
    VideoWindow_put_Height,
    VideoWindow_get_Height,
    VideoWindow_put_Owner,
    VideoWindow_get_Owner,
    VideoWindow_put_MessageDrain,
    VideoWindow_get_MessageDrain,
    VideoWindow_get_BorderColor,
    VideoWindow_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    VideoWindow_SetWindowForeground,
    VideoWindow_NotifyOwnerMessage,
    VideoWindow_SetWindowPosition,
    VideoWindow_GetWindowPosition,
    VideoWindow_GetMinIdealImageSize,
    VideoWindow_GetMaxIdealImageSize,
    VideoWindow_GetRestorePosition,
    VideoWindow_HideCursor,
    VideoWindow_IsCursorHidden
};


/*** IUnknown methods ***/
static HRESULT WINAPI MediaEvent_QueryInterface(IMediaEventEx *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaEvent_AddRef(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaEvent_Release(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaEvent_GetTypeInfoCount(IMediaEventEx *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetTypeInfo(IMediaEventEx *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetIDsOfNames(IMediaEventEx *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_Invoke(IMediaEventEx *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IMediaEvent methods ***/
static HRESULT WINAPI MediaEvent_GetEventHandle(IMediaEventEx *iface,
						OAEVENT *hEvent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, hEvent);

    *hEvent = (OAEVENT)This->evqueue.msg_event;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetEvent(IMediaEventEx *iface,
					  long *lEventCode,
					  LONG_PTR *lParam1,
					  LONG_PTR *lParam2,
					  long msTimeout) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);
    Event evt;

    TRACE("(%p/%p)->(%p, %p, %p, %ld)\n", This, iface, lEventCode, lParam1, lParam2, msTimeout);

    if (EventsQueue_GetEvent(&This->evqueue, &evt, msTimeout))
    {
	*lEventCode = evt.lEventCode;
	*lParam1 = evt.lParam1;
	*lParam2 = evt.lParam2;
	return S_OK;
    }

    *lEventCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_WaitForCompletion(IMediaEventEx *iface,
						   long msTimeout,
						   long *pEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %p)\n", This, iface, msTimeout, pEvCode);

    if (WaitForSingleObject(This->hEventCompletion, msTimeout) == WAIT_OBJECT_0)
    {
	*pEvCode = This->CompletionStatus;
	return S_OK;
    }

    *pEvCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_CancelDefaultHandling(IMediaEventEx *iface,
						       long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = FALSE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = FALSE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = FALSE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_RestoreDefaultHandling(IMediaEventEx *iface,
							long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = TRUE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = TRUE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = TRUE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_FreeEventParams(IMediaEventEx *iface,
						 long lEvCode,
						 LONG_PTR lParam1,
						 LONG_PTR lParam2) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %08lx, %08lx): stub !!!\n", This, iface, lEvCode, lParam1, lParam2);

    return S_OK;
}

/*** IMediaEventEx methods ***/
static HRESULT WINAPI MediaEvent_SetNotifyWindow(IMediaEventEx *iface,
						 OAHWND hwnd,
						 long lMsg,
						 LONG_PTR lInstanceData) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%08x, %ld, %08lx)\n", This, iface, (DWORD) hwnd, lMsg, lInstanceData);

    This->notif.hWnd = (HWND)hwnd;
    This->notif.msg = lMsg;
    This->notif.instance = (long) lInstanceData;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_SetNotifyFlags(IMediaEventEx *iface,
						long lNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lNoNotifyFlags);

    if ((lNoNotifyFlags != 0) && (lNoNotifyFlags != 1))
	return E_INVALIDARG;

    This->notif.disabled = lNoNotifyFlags;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetNotifyFlags(IMediaEventEx *iface,
						long *lplNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, lplNoNotifyFlags);

    if (!lplNoNotifyFlags)
	return E_POINTER;

    *lplNoNotifyFlags = This->notif.disabled;

    return S_OK;
}


static const IMediaEventExVtbl IMediaEventEx_VTable =
{
    MediaEvent_QueryInterface,
    MediaEvent_AddRef,
    MediaEvent_Release,
    MediaEvent_GetTypeInfoCount,
    MediaEvent_GetTypeInfo,
    MediaEvent_GetIDsOfNames,
    MediaEvent_Invoke,
    MediaEvent_GetEventHandle,
    MediaEvent_GetEvent,
    MediaEvent_WaitForCompletion,
    MediaEvent_CancelDefaultHandling,
    MediaEvent_RestoreDefaultHandling,
    MediaEvent_FreeEventParams,
    MediaEvent_SetNotifyWindow,
    MediaEvent_SetNotifyFlags,
    MediaEvent_GetNotifyFlags
};


static HRESULT WINAPI MediaFilter_QueryInterface(IMediaFilter *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI MediaFilter_AddRef(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaFilter_Release(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI MediaFilter_GetClassID(IMediaFilter *iface, CLSID * pClassID)
{
    FIXME("(%p): stub\n", pClassID);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Stop(IMediaFilter *iface)
{
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Pause(IMediaFilter *iface)
{
    FIXME("(): stub\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Run(IMediaFilter *iface, REFERENCE_TIME tStart)
{
    FIXME("(0x%s): stub\n", wine_dbgstr_longlong(tStart));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_GetState(IMediaFilter *iface, DWORD dwMsTimeout, FILTER_STATE * pState)
{
    FIXME("(%d, %p): stub\n", dwMsTimeout, pState);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_SetSyncSource(IMediaFilter *iface, IReferenceClock *pClock)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);
    HRESULT hr = S_OK;
    int i;

    TRACE("(%p/%p)->(%p)\n", iface, This, pClock);

    EnterCriticalSection(&This->cs);
    {
        for (i = 0;i < This->nFilters;i++)
        {
            hr = IBaseFilter_SetSyncSource(This->ppFiltersInGraph[i], pClock);
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
        {
            for(;i >= 0;i--)
                IBaseFilter_SetSyncSource(This->ppFiltersInGraph[i], This->refClock);
        }
        else
        {
            if (This->refClock)
                IReferenceClock_Release(This->refClock);
            This->refClock = pClock;
            if (This->refClock)
                IReferenceClock_AddRef(This->refClock);

            if (This->HandleEcClockChanged)
            {
                IMediaEventSink *pEventSink;
                HRESULT eshr;

                eshr = IMediaFilter_QueryInterface(iface, &IID_IMediaEventSink, (LPVOID)&pEventSink);
                if (SUCCEEDED(eshr))
                {
                    IMediaEventSink_Notify(pEventSink, EC_CLOCK_CHANGED, 0, 0);
                    IMediaEventSink_Release(pEventSink);
                }
            }
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaFilter_GetSyncSource(IMediaFilter *iface, IReferenceClock **ppClock)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaFilter_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, ppClock);

    if (!ppClock)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    {
        *ppClock = This->refClock;
        if (*ppClock)
            IReferenceClock_AddRef(*ppClock);
    }
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static const IMediaFilterVtbl IMediaFilter_VTable =
{
    MediaFilter_QueryInterface,
    MediaFilter_AddRef,
    MediaFilter_Release,
    MediaFilter_GetClassID,
    MediaFilter_Stop,
    MediaFilter_Pause,
    MediaFilter_Run,
    MediaFilter_GetState,
    MediaFilter_SetSyncSource,
    MediaFilter_GetSyncSource
};

static HRESULT WINAPI MediaEventSink_QueryInterface(IMediaEventSink *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI MediaEventSink_AddRef(IMediaEventSink *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaEventSink_Release(IMediaEventSink *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI MediaEventSink_Notify(IMediaEventSink *iface, long EventCode, LONG_PTR EventParam1, LONG_PTR EventParam2)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);
    Event evt;

    TRACE("(%p/%p)->(%ld, %ld, %ld)\n", This, iface, EventCode, EventParam1, EventParam2);

    /* We need thread safety here, let's use the events queue's one */
    EnterCriticalSection(&This->evqueue.msg_crst);

    if ((EventCode == EC_COMPLETE) && This->HandleEcComplete)
    {
        TRACE("Process EC_COMPLETE notification\n");
        if (++This->EcCompleteCount == This->nRenderers)
        {
            evt.lEventCode = EC_COMPLETE;
            evt.lParam1 = S_OK;
            evt.lParam2 = 0;
            TRACE("Send EC_COMPLETE to app\n");
            EventsQueue_PutEvent(&This->evqueue, &evt);
            if (!This->notif.disabled && This->notif.hWnd)
	    {
                TRACE("Send Window message\n");
                PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
            }
            This->CompletionStatus = EC_COMPLETE;
            SetEvent(This->hEventCompletion);
        }
    }
    else if ((EventCode == EC_REPAINT) && This->HandleEcRepaint)
    {
        /* FIXME: Not handled yet */
    }
    else
    {
        evt.lEventCode = EventCode;
        evt.lParam1 = EventParam1;
        evt.lParam2 = EventParam2;
        EventsQueue_PutEvent(&This->evqueue, &evt);
        if (!This->notif.disabled && This->notif.hWnd)
            PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
    }

    LeaveCriticalSection(&This->evqueue.msg_crst);
    return S_OK;
}

static const IMediaEventSinkVtbl IMediaEventSink_VTable =
{
    MediaEventSink_QueryInterface,
    MediaEventSink_AddRef,
    MediaEventSink_Release,
    MediaEventSink_Notify
};

static HRESULT WINAPI GraphConfig_QueryInterface(IGraphConfig *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI GraphConfig_AddRef(IGraphConfig *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventSink_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI GraphConfig_Release(IGraphConfig *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    return Filtergraph_Release(This);
}

static HRESULT WINAPI GraphConfig_Reconnect(IGraphConfig *iface,
					    IPin* pOutputPin,
					    IPin* pInputPin,
					    const AM_MEDIA_TYPE* pmtFirstConnection,
					    IBaseFilter* pUsingFilter,
					    HANDLE hAbortEvent,
					    DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p, %x): stub!\n", This, pOutputPin, pInputPin, pmtFirstConnection, pUsingFilter, hAbortEvent, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_Reconfigure(IGraphConfig *iface,
					      IGraphConfigCallback* pCallback,
					      PVOID pvContext,
					      DWORD dwFlags,
					      HANDLE hAbortEvent)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p, %x, %p): stub!\n", This, pCallback, pvContext, dwFlags, hAbortEvent);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_AddFilterToCache(IGraphConfig *iface,
						   IBaseFilter* pFilter)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_EnumCacheFilter(IGraphConfig *iface,
						  IEnumFilters** pEnum)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pEnum);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterFromCache(IGraphConfig *iface,
							IBaseFilter* pFilter)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetStartTime(IGraphConfig *iface,
					       REFERENCE_TIME* prtStart)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p): stub!\n", This, prtStart);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_PushThroughData(IGraphConfig *iface,
						  IPin* pOutputPin,
						  IPinConnection* pConnection,
						  HANDLE hEventAbort)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p, %p): stub!\n", This, pOutputPin, pConnection, hEventAbort);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_SetFilterFlags(IGraphConfig *iface,
						 IBaseFilter* pFilter,
						 DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetFilterFlags(IGraphConfig *iface,
						 IBaseFilter* pFilter,
						 DWORD* dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterEx(IGraphConfig *iface,
					  	 IBaseFilter* pFilter,
						 DWORD dwFlags)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphConfig_vtbl, iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);
    
    return E_NOTIMPL;
}

static const IGraphConfigVtbl IGraphConfig_VTable =
{
    GraphConfig_QueryInterface,
    GraphConfig_AddRef,
    GraphConfig_Release,
    GraphConfig_Reconnect,
    GraphConfig_Reconfigure,
    GraphConfig_AddFilterToCache,
    GraphConfig_EnumCacheFilter,
    GraphConfig_RemoveFilterFromCache,
    GraphConfig_GetStartTime,
    GraphConfig_PushThroughData,
    GraphConfig_SetFilterFlags,
    GraphConfig_GetFilterFlags,
    GraphConfig_RemoveFilterEx
};

/* This is the only function that actually creates a FilterGraph class... */
HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IFilterGraphImpl *fimpl;
    HRESULT hr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    fimpl->IGraphBuilder_vtbl = &IGraphBuilder_VTable;
    fimpl->IMediaControl_vtbl = &IMediaControl_VTable;
    fimpl->IMediaSeeking_vtbl = &IMediaSeeking_VTable;
    fimpl->IBasicAudio_vtbl = &IBasicAudio_VTable;
    fimpl->IBasicVideo_vtbl = &IBasicVideo_VTable;
    fimpl->IVideoWindow_vtbl = &IVideoWindow_VTable;
    fimpl->IMediaEventEx_vtbl = &IMediaEventEx_VTable;
    fimpl->IMediaFilter_vtbl = &IMediaFilter_VTable;
    fimpl->IMediaEventSink_vtbl = &IMediaEventSink_VTable;
    fimpl->IGraphConfig_vtbl = &IGraphConfig_VTable;
    fimpl->IMediaPosition_vtbl = &IMediaPosition_VTable;
    fimpl->ref = 1;
    fimpl->ppFiltersInGraph = NULL;
    fimpl->pFilterNames = NULL;
    fimpl->nFilters = 0;
    fimpl->filterCapacity = 0;
    fimpl->nameIndex = 1;
    fimpl->refClock = NULL;
    fimpl->hEventCompletion = CreateEventW(0, TRUE, FALSE, 0);
    fimpl->HandleEcComplete = TRUE;
    fimpl->HandleEcRepaint = TRUE;
    fimpl->HandleEcClockChanged = TRUE;
    fimpl->notif.hWnd = 0;
    fimpl->notif.disabled = FALSE;
    fimpl->nRenderers = 0;
    fimpl->EcCompleteCount = 0;
    fimpl->state = State_Stopped;
    EventsQueue_Init(&fimpl->evqueue);
    InitializeCriticalSection(&fimpl->cs);
    fimpl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IFilterGraphImpl.cs");
    fimpl->nItfCacheEntries = 0;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&fimpl->pFilterMapper2);
    if (FAILED(hr)) {
        ERR("Unable to create filter mapper (%x)\n", hr);
	return hr;
    }

    *ppObj = fimpl;
    return S_OK;
}

HRESULT FilterGraphNoThread_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    FIXME("CLSID_FilterGraphNoThread partially implemented - Forwarding to CLSID_FilterGraph\n");
    return FilterGraph_create(pUnkOuter, ppObj);
}
