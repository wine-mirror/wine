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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#define COM_NO_WINDOWS_H
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
    omr->messages = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,omr->ring_buffer_size * sizeof(Event));

    InitializeCriticalSection(&omr->msg_crst);
    return TRUE;
}

static int EventsQueue_Destroy(EventsQueue* omr)
{
    CloseHandle(omr->msg_event);
    HeapFree(GetProcessHeap(),0,omr->messages);
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

typedef struct _IFilterGraphImpl {
    IGraphBuilderVtbl *IGraphBuilder_vtbl;
    IMediaControlVtbl *IMediaControl_vtbl;
    IMediaSeekingVtbl *IMediaSeeking_vtbl;
    IBasicAudioVtbl *IBasicAudio_vtbl;
    IBasicVideoVtbl *IBasicVideo_vtbl;
    IVideoWindowVtbl *IVideoWindow_vtbl;
    IMediaEventExVtbl *IMediaEventEx_vtbl;
    IMediaFilterVtbl *IMediaFilter_vtbl;
    IMediaEventSinkVtbl *IMediaEventSink_vtbl;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IBasicVideo2 */
    /* IFilterChain */
    /* IFilterGraph2 */
    /* IFilterMapper2 */
    /* IGraphConfig */
    /* IGraphVersion */
    /* IMediaPosition */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceMananger */
    /* IServiceProvider */
    /* IVideoFrameStep */

    ULONG ref;
    IFilterMapper2 * pFilterMapper2;
    IBaseFilter ** ppFiltersInGraph;
    LPWSTR * pFilterNames;
    int nFilters;
    int filterCapacity;
    long nameIndex;
    EventsQueue evqueue;
    HANDLE hEventCompletion;
    int CompletionStatus;
    WndNotify notif;
    int nRenderers;
    int EcCompleteCount;
    int HandleEcComplete;
    int HandleEcRepaint;
    OAFilterState state;
    CRITICAL_SECTION cs;
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

    TRACE("(%p)->(): new ref = %ld\n", This, ref);
    
    return ref;
}

static ULONG Filtergraph_Release(IFilterGraphImpl *This) {
    ULONG ref = InterlockedDecrement(&This->ref);
    
    TRACE("(%p)->(): new ref = %ld\n", This, ref);
    
    if (ref == 0) {
        int i;
        for (i = 0; i < This->nFilters; i++)
           IBaseFilter_Release(This->ppFiltersInGraph[i]);
	IFilterMapper2_Release(This->pFilterMapper2);
	CloseHandle(This->hEventCompletion);
	EventsQueue_Destroy(&This->evqueue);
	DeleteCriticalSection(&This->cs);
	HeapFree(GetProcessHeap(), 0, This->ppFiltersInGraph);
	HeapFree(GetProcessHeap(), 0, This->pFilterNames);
	HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}


/*** IUnknown methods ***/
static HRESULT WINAPI Graphbuilder_QueryInterface(IGraphBuilder *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);
    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Graphbuilder_AddRef(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph AddRef\n", This, iface);
    
    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Graphbuilder_Release(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    
    TRACE("(%p/%p)->() calling FilterGraph Release\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IFilterGraph methods ***/
static HRESULT WINAPI Graphbuilder_AddFilter(IGraphBuilder *iface,
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
        int newCapacity = 2*This->filterCapacity;
        IBaseFilter ** ppNewFilters = CoTaskMemAlloc(newCapacity * sizeof(IBaseFilter*));
        LPWSTR * pNewNames = CoTaskMemAlloc(newCapacity * sizeof(LPWSTR));
        memcpy(ppNewFilters, This->ppFiltersInGraph, This->nFilters * sizeof(IBaseFilter*));
        memcpy(pNewNames, This->pFilterNames, This->nFilters * sizeof(LPWSTR));
        CoTaskMemFree(This->ppFiltersInGraph);
        CoTaskMemFree(This->pFilterNames);
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
    }
    else
	CoTaskMemFree(wszFilterName);

    if (SUCCEEDED(hr) && duplicate_name)
	return VFW_S_DUPLICATE_NAME;
	
    return hr;
}

static HRESULT WINAPI Graphbuilder_RemoveFilter(IGraphBuilder *iface,
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
            /* FIXME: disconnect pins */
            hr = IBaseFilter_JoinFilterGraph(pFilter, NULL, This->pFilterNames[i]);
            if (SUCCEEDED(hr))
            {
                IPin_Release(pFilter);
                CoTaskMemFree(This->pFilterNames[i]);
                memmove(This->ppFiltersInGraph+i, This->ppFiltersInGraph+i+1, sizeof(IBaseFilter*)*(This->nFilters - 1 - i));
                memmove(This->pFilterNames+i, This->pFilterNames+i+1, sizeof(LPWSTR)*(This->nFilters - 1 - i));
                This->nFilters--;
                return S_OK;
            }
            break;
        }
    }

    return hr; /* FIXME: check this error code */
}

static HRESULT WINAPI Graphbuilder_EnumFilters(IGraphBuilder *iface,
					      IEnumFilters **ppEnum) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumFiltersImpl_Construct(This->ppFiltersInGraph, This->nFilters, ppEnum);
}

static HRESULT WINAPI Graphbuilder_FindFilterByName(IGraphBuilder *iface,
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
static HRESULT WINAPI Graphbuilder_ConnectDirect(IGraphBuilder *iface,
						 IPin *ppinIn,
						 IPin *ppinOut,
						 const AM_MEDIA_TYPE *pmt) {
    PIN_DIRECTION dir;
    HRESULT hr;

    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);

    /* FIXME: check pins are in graph */

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

static HRESULT WINAPI Graphbuilder_Reconnect(IGraphBuilder *iface,
					     IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    IPin *pConnectedTo = NULL;
    HRESULT hr;
    PIN_DIRECTION pindir;

    IPin_QueryDirection(ppin, &pindir);
    hr = IPin_ConnectedTo(ppin, &pConnectedTo);
    if (FAILED(hr)) {
        TRACE("Querying connected to failed: %lx\n", hr);
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
    TRACE("(%p->%p) -- %p %p -> %lx\n", iface, This, ppin, pConnectedTo, hr);
    return hr;
}

static HRESULT WINAPI Graphbuilder_Disconnect(IGraphBuilder *iface,
					      IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppin);

    return IPin_Disconnect(ppin);
}

static HRESULT WINAPI Graphbuilder_SetDefaultSyncSource(IGraphBuilder *iface) {
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
            ERR("Error (%lx)\n", hr);
        }
    } else if (hr == E_NOTIMPL) {
        /* Input connected to all outputs */
        IEnumPins* penumpins;
        IPin* ppin;
        int i = 0;
        TRACE("E_NOTIMPL\n");
        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            ERR("filter Enumpins failed (%lx)\n", hr);
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
        nb = i;
        if (FAILED(hr)) {
            ERR("Next failed (%lx)\n", hr);
            return hr;
        }
        IEnumPins_Release(penumpins);
    } else if (FAILED(hr)) {
        ERR("Cannot get internal connection (%lx)\n", hr);
        return hr;
    }

    *pnb = nb;
    return S_OK;
}

/*** IGraphBuilder methods ***/
static HRESULT WINAPI Graphbuilder_Connect(IGraphBuilder *iface,
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

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, ppinOut, ppinIn);

    /* Try direct connection first */
    hr = IPin_Connect(ppinOut, ppinIn, NULL);
    if (SUCCEEDED(hr)) {
        return S_OK;
    }
    TRACE("Direct connection failed, trying to insert other filters\n");

    /* Find the appropriate transform filter than can transform the minor media type of output pin of the upstream 
     * filter to the minor mediatype of input pin of the renderer */
    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        ERR("EnumMediaTypes (%lx)\n", hr);
        return hr;
    }

    hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
    if (FAILED(hr)) {
        ERR("IEnumMediaTypes_Next (%lx)\n", hr);
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
        ERR("Unable to enum filters (%lx)\n", hr);
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
           ERR("Unable to retrieve filter info (%lx)\n", hr);
           goto error;
        }

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
        if (FAILED(hr)) {
           ERR("Unable to create filter (%lx), trying next one\n", hr);
           goto error;
        }

        hr = IGraphBuilder_AddFilter(iface, pfilter, NULL);
        if (FAILED(hr)) {
            ERR("Unable to add filter (%lx)\n", hr);
            IBaseFilter_Release(pfilter);
            pfilter = NULL;
            goto error;
        }

        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            ERR("Enumpins (%lx)\n", hr);
            goto error;
        }

        hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin);
        if (FAILED(hr)) {
            ERR("Next (%lx)\n", hr);
            goto error;
        }
        if (pin == 0) {
            ERR("No Pin\n");
            goto error;
        }
        IEnumPins_Release(penumpins);

        hr = IPin_Connect(ppinOut, ppinfilter, NULL);
        if (FAILED(hr)) {
            TRACE("Cannot connect to filter (%lx), trying next one\n", hr);
            goto error;
        }
        TRACE("Successfully connected to filter, follow chain...\n");

        /* Render all output pins of the filter by calling IGraphBuilder_Render on each of them */
        hr = GetInternalConnections(pfilter, ppinfilter, &ppins, &nb);

        if (SUCCEEDED(hr)) {
            int i;
            TRACE("pins to consider: %ld\n", nb);
            for(i = 0; i < nb; i++) {
                TRACE("Processing pin %d\n", i);
                hr = IGraphBuilder_Connect(iface, ppins[i], ppinIn);
                if (FAILED(hr)) {
                   TRACE("Cannot render pin %p (%lx)\n", ppinfilter, hr);
                }
                IPin_Release(ppins[i]);
                if (SUCCEEDED(hr)) break;
            }
            while (++i < nb) IPin_Release(ppins[i]);
            CoTaskMemFree(ppins);
            IBaseFilter_Release(pfilter);
            IPin_Release(ppinfilter);
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

static HRESULT WINAPI Graphbuilder_Render(IGraphBuilder *iface,
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

    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        ERR("EnumMediaTypes (%lx)\n", hr);
        return hr;
    }

    while(1)
    {
        hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
        if (FAILED(hr)) {
            ERR("IEnumMediaTypes_Next (%lx)\n", hr);
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
            ERR("Unable to enum filters (%lx)\n", hr);
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
                ERR("Unable to retrieve filter info (%lx)\n", hr);
                goto error;
            }

            hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pfilter);
            if (FAILED(hr)) {
               ERR("Unable to create filter (%lx), trying next one\n", hr);
               goto error;
            }

            hr = IGraphBuilder_AddFilter(iface, pfilter, NULL);
            if (FAILED(hr)) {
                ERR("Unable to add filter (%lx)\n", hr);
                IBaseFilter_Release(pfilter);
                pfilter = NULL;
                goto error;
            }

            hr = IBaseFilter_EnumPins(pfilter, &penumpins);
            if (FAILED(hr)) {
                ERR("Splitter Enumpins (%lx)\n", hr);
                goto error;
            }
            hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin);
            if (FAILED(hr)) {
               ERR("Next (%lx)\n", hr);
               goto error;
            }
            if (pin == 0) {
               ERR("No Pin\n");
               goto error;
            }
            IEnumPins_Release(penumpins);

	    /* Connect the pin to render to the renderer */
            hr = IGraphBuilder_Connect(iface, ppinOut, ppinfilter);
            if (FAILED(hr)) {
                TRACE("Unable to connect to renderer (%lx)\n", hr);
                goto error;
            }
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

static HRESULT WINAPI Graphbuilder_RenderFile(IGraphBuilder *iface,
					      LPCWSTR lpcwstrFile,
					      LPCWSTR lpcwstrPlayList) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);
    static const WCHAR string[] = {'R','e','a','d','e','r',0};
    IBaseFilter* preader = NULL;
    IBaseFilter* psplitter;
    IPin* ppinreader;
    IPin* ppinsplitter;
    IEnumPins* penumpins;
    ULONG pin;
    HRESULT hr;
    IEnumMoniker* pEnumMoniker;
    GUID tab[2];
    IPin** ppins;
    ULONG nb;
    IMoniker* pMoniker;
    IFileSourceFilter* pfile = NULL;
    AM_MEDIA_TYPE mt;
    WCHAR* filename;

    TRACE("(%p/%p)->(%s, %s)\n", This, iface, debugstr_w(lpcwstrFile), debugstr_w(lpcwstrPlayList));

    hr = IGraphBuilder_AddSourceFilter(iface, lpcwstrFile, string, &preader);

    /* Retrieve file media type */
    if (SUCCEEDED(hr))
        hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter, (LPVOID*)&pfile);
    if (SUCCEEDED(hr)) {
        hr = IFileSourceFilter_GetCurFile(pfile, &filename, &mt);
        IFileSourceFilter_Release(pfile);
    }

    if (SUCCEEDED(hr)) {
        tab[0] = mt.majortype;
        tab[1] = mt.subtype;
        hr = IFilterMapper2_EnumMatchingFilters(This->pFilterMapper2, &pEnumMoniker, 0, FALSE, 0, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
    }

    if (FAILED(hr))
    {
        if (preader) {
             IGraphBuilder_RemoveFilter(iface, preader);
             IBaseFilter_Release(preader);
        }
        return hr;
    }

    hr = E_FAIL;
    while(IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
    {
        VARIANT var;
        GUID clsid;

        hr = GetFilterInfo(pMoniker, &clsid, &var);
        IMoniker_Release(pMoniker);
        if (FAILED(hr)) {
            ERR("Unable to retrieve filter info (%lx)\n", hr);
            continue;
        }

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&psplitter);
        if (FAILED(hr)) {
           ERR("Unable to create filter (%lx), trying next one\n", hr);
           continue;
        }

        hr = IGraphBuilder_AddFilter(iface, psplitter, NULL);
        if (FAILED(hr)) {
            ERR("Unable add filter (%lx)\n", hr);
            return hr;
        }

        /* Connect file source and splitter filters together */
        /* Make the splitter analyze incoming data */
        hr = IBaseFilter_EnumPins(preader, &penumpins);
        if (FAILED(hr)) {
            ERR("Enumpins (%lx)\n", hr);
            return hr;
        }
        hr = IEnumPins_Next(penumpins, 1, &ppinreader, &pin);
        if (FAILED(hr)) {
            ERR("Next (%lx)\n", hr);
            return hr;
        }
        if (pin == 0) {
            ERR("No Pin\n");
            return E_FAIL;
        }
        IEnumPins_Release(penumpins);

        hr = IBaseFilter_EnumPins(psplitter, &penumpins);
        if (FAILED(hr)) {
            ERR("Splitter Enumpins (%lx)\n", hr);
            return hr;
        }
        hr = IEnumPins_Next(penumpins, 1, &ppinsplitter, &pin);
        if (FAILED(hr)) {
            ERR("Next (%lx)\n", hr);
            return hr;
        }
        if (pin == 0) {
            ERR("No Pin\n");
            return E_FAIL;
        }
        IEnumPins_Release(penumpins);

        hr = IPin_Connect(ppinreader, ppinsplitter, NULL);
        if (FAILED(hr)) {
            IBaseFilter_Release(ppinsplitter);
            ppinsplitter = NULL;
            TRACE("Cannot connect to filter (%lx), trying next one\n", hr);
            break;
        }
        TRACE("Successfully connected to filter\n");
        break;
    }

    /* Render all output pin of the splitter by calling IGraphBuilder_Render on each of them */
    if (SUCCEEDED(hr))
        hr = GetInternalConnections(psplitter, ppinsplitter, &ppins, &nb);
    
    if (SUCCEEDED(hr)) {
        int i;
        TRACE("pins to consider: %ld\n", nb);
        for(i = 0; i < nb; i++) {
            TRACE("Processing pin %d\n", i);
            hr = IGraphBuilder_Render(iface, ppins[i]);
            if (FAILED(hr)) {
                ERR("Cannot render pin %p (%lx)\n", ppins[i], hr);
                /* FIXME: We should clean created things properly */
                break;
            }
        }
        CoTaskMemFree(ppins);
    }
    
    return hr;
}

static HRESULT WINAPI Graphbuilder_AddSourceFilter(IGraphBuilder *iface,
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
        ERR("Unable to create file source filter (%lx)\n", hr);
        return hr;
    }

    hr = IGraphBuilder_AddFilter(iface, preader, lpcwstrFilterName);
    if (FAILED(hr)) {
        ERR("Unable add filter (%lx)\n", hr);
        IBaseFilter_Release(preader);
        return hr;
    }

    hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter, (LPVOID*)&pfile);
    if (FAILED(hr)) {
        ERR("Unable to get IFileSourceInterface (%lx)\n", hr);
        goto error;
    }

    /* Load the file in the file source filter */
    hr = IFileSourceFilter_Load(pfile, lpcwstrFileName, NULL);
    if (FAILED(hr)) {
        ERR("Load (%lx)\n", hr);
        goto error;
    }
    
    IFileSourceFilter_GetCurFile(pfile, &filename, &mt);
    if (FAILED(hr)) {
        ERR("GetCurFile (%lx)\n", hr);
        goto error;
    }
    TRACE("File %s\n", debugstr_w(filename));
    TRACE("MajorType %s\n", debugstr_guid(&mt.majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt.subtype));

    if (ppFilter)
        *ppFilter = preader;

    return S_OK;
    
error:
    if (pfile)
        IFileSourceFilter_Release(pfile);
    IGraphBuilder_RemoveFilter(iface, preader);
    IBaseFilter_Release(preader);
       
    return hr;
}

static HRESULT WINAPI Graphbuilder_SetLogFile(IGraphBuilder *iface,
					      DWORD_PTR hFile) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%08lx): stub !!!\n", This, iface, (DWORD) hFile);

    return S_OK;
}

static HRESULT WINAPI Graphbuilder_Abort(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Graphbuilder_ShouldOperationContinue(IGraphBuilder *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static IGraphBuilderVtbl IGraphBuilder_VTable =
{
    Graphbuilder_QueryInterface,
    Graphbuilder_AddRef,
    Graphbuilder_Release,
    Graphbuilder_AddFilter,
    Graphbuilder_RemoveFilter,
    Graphbuilder_EnumFilters,
    Graphbuilder_FindFilterByName,
    Graphbuilder_ConnectDirect,
    Graphbuilder_Reconnect,
    Graphbuilder_Disconnect,
    Graphbuilder_SetDefaultSyncSource,
    Graphbuilder_Connect,
    Graphbuilder_Render,
    Graphbuilder_RenderFile,
    Graphbuilder_AddSourceFilter,
    Graphbuilder_SetLogFile,
    Graphbuilder_Abort,
    Graphbuilder_ShouldOperationContinue
};

/*** IUnknown methods ***/
static HRESULT WINAPI Mediacontrol_QueryInterface(IMediaControl *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Mediacontrol_AddRef(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Mediacontrol_Release(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);

}

/*** IDispatch methods ***/
static HRESULT WINAPI Mediacontrol_GetTypeInfoCount(IMediaControl *iface,
						    UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_GetTypeInfo(IMediaControl *iface,
					       UINT iTInfo,
					       LCID lcid,
					       ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%d, %ld, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_GetIDsOfNames(IMediaControl *iface,
						 REFIID riid,
						 LPOLESTR*rgszNames,
						 UINT cNames,
						 LCID lcid,
						 DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %ld, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_Invoke(IMediaControl *iface,
					  DISPID dispIdMember,
					  REFIID riid,
					  LCID lcid,
					  WORD wFlags,
					  DISPPARAMS*pDispParams,
					  VARIANT*pVarResult,
					  EXCEPINFO*pExepInfo,
					  UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %s (%p), %ld, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

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
        hr = IPin_QueryPinInfo(pInputPin, &PinInfo);

    if (SUCCEEDED(hr))
        hr = GetInternalConnections(PinInfo.pFilter, pInputPin, &ppPins, &nb);

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
    LONG dummy;
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
            ERR("Enum pins failed %lx\n", hr);
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
        if (source == TRUE)
        {
            TRACE("Found a source filter\n");
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
static HRESULT WINAPI Mediacontrol_Run(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Running) return S_OK;

    EnterCriticalSection(&This->cs);
    SendFilterMessage(iface, SendRun);
    This->state = State_Running;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI Mediacontrol_Pause(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);
    TRACE("(%p/%p)->()\n", This, iface);

    if (This->state == State_Paused) return S_OK;

    EnterCriticalSection(&This->cs);
    SendFilterMessage(iface, SendPause);
    This->state = State_Paused;
    LeaveCriticalSection(&This->cs);
    return S_FALSE;
}

static HRESULT WINAPI Mediacontrol_Stop(IMediaControl *iface) {
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

static HRESULT WINAPI Mediacontrol_GetState(IMediaControl *iface,
					    LONG msTimeout,
					    OAFilterState *pfs) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %p): semi-stub !!!\n", This, iface, msTimeout, pfs);

    EnterCriticalSection(&This->cs);

    *pfs = This->state;

    LeaveCriticalSection(&This->cs);
    
    return S_OK;
}

static HRESULT WINAPI Mediacontrol_RenderFile(IMediaControl *iface,
					      BSTR strFilename) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p)): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_AddSourceFilter(IMediaControl *iface,
						   BSTR strFilename,
						   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename, ppUnk);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_get_FilterCollection(IMediaControl *iface,
							IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_get_RegFilterCollection(IMediaControl *iface,
							   IDispatch **ppUnk) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_StopWhenReady(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static IMediaControlVtbl IMediaControl_VTable =
{
    Mediacontrol_QueryInterface,
    Mediacontrol_AddRef,
    Mediacontrol_Release,
    Mediacontrol_GetTypeInfoCount,
    Mediacontrol_GetTypeInfo,
    Mediacontrol_GetIDsOfNames,
    Mediacontrol_Invoke,
    Mediacontrol_Run,
    Mediacontrol_Pause,
    Mediacontrol_Stop,
    Mediacontrol_GetState,
    Mediacontrol_RenderFile,
    Mediacontrol_AddSourceFilter,
    Mediacontrol_get_FilterCollection,
    Mediacontrol_get_RegFilterCollection,
    Mediacontrol_StopWhenReady
};


/*** IUnknown methods ***/
static HRESULT WINAPI Mediaseeking_QueryInterface(IMediaSeeking *iface,
						  REFIID riid,
						  LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Mediaseeking_AddRef(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Mediaseeking_Release(IMediaSeeking *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IMediaSeeking methods ***/
static HRESULT WINAPI Mediaseeking_GetCapabilities(IMediaSeeking *iface,
						   DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCapabilities);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_CheckCapabilities(IMediaSeeking *iface,
						     DWORD *pCapabilities) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCapabilities);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_IsFormatSupported(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_QueryPreferredFormat(IMediaSeeking *iface,
							GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetTimeFormat(IMediaSeeking *iface,
						 GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_IsUsingTimeFormat(IMediaSeeking *iface,
						     const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_SetTimeFormat(IMediaSeeking *iface,
						 const GUID *pFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetDuration(IMediaSeeking *iface,
					       LONGLONG *pDuration) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDuration);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetStopPosition(IMediaSeeking *iface,
						   LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pStop);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetCurrentPosition(IMediaSeeking *iface,
						      LONGLONG *pCurrent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pCurrent);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_ConvertTimeFormat(IMediaSeeking *iface,
						     LONGLONG *pTarget,
						     const GUID *pTargetFormat,
						     LONGLONG Source,
						     const GUID *pSourceFormat) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %lld, %p): stub !!!\n", This, iface, pTarget, pTargetFormat, Source, pSourceFormat);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_SetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						DWORD dwCurrentFlags,
						LONGLONG *pStop,
						DWORD dwStopFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %08lx, %p, %08lx): stub !!!\n", This, iface, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetPositions(IMediaSeeking *iface,
						LONGLONG *pCurrent,
						LONGLONG *pStop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pCurrent, pStop);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetAvailable(IMediaSeeking *iface,
						LONGLONG *pEarliest,
						LONGLONG *pLatest) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pEarliest, pLatest);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_SetRate(IMediaSeeking *iface,
					   double dRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%f): stub !!!\n", This, iface, dRate);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetRate(IMediaSeeking *iface,
					   double *pdRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pdRate);

    return S_OK;
}

static HRESULT WINAPI Mediaseeking_GetPreroll(IMediaSeeking *iface,
					      LONGLONG *pllPreroll) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pllPreroll);

    return S_OK;
}


static IMediaSeekingVtbl IMediaSeeking_VTable =
{
    Mediaseeking_QueryInterface,
    Mediaseeking_AddRef,
    Mediaseeking_Release,
    Mediaseeking_GetCapabilities,
    Mediaseeking_CheckCapabilities,
    Mediaseeking_IsFormatSupported,
    Mediaseeking_QueryPreferredFormat,
    Mediaseeking_GetTimeFormat,
    Mediaseeking_IsUsingTimeFormat,
    Mediaseeking_SetTimeFormat,
    Mediaseeking_GetDuration,
    Mediaseeking_GetStopPosition,
    Mediaseeking_GetCurrentPosition,
    Mediaseeking_ConvertTimeFormat,
    Mediaseeking_SetPositions,
    Mediaseeking_GetPositions,
    Mediaseeking_GetAvailable,
    Mediaseeking_SetRate,
    Mediaseeking_GetRate,
    Mediaseeking_GetPreroll
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicaudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Basicaudio_AddRef(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Basicaudio_Release(IBasicAudio *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Basicaudio_GetTypeInfoCount(IBasicAudio *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_GetTypeInfo(IBasicAudio *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%d, %ld, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_GetIDsOfNames(IBasicAudio *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %ld, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_Invoke(IBasicAudio *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %s (%p), %ld, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI Basicaudio_put_Volume(IBasicAudio *iface,
					    long lVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, lVolume);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Volume(IBasicAudio *iface,
					    long *plVolume) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, plVolume);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_put_Balance(IBasicAudio *iface,
					     long lBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, lBalance);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Balance(IBasicAudio *iface,
					     long *plBalance) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, plBalance);

    return S_OK;
}

static IBasicAudioVtbl IBasicAudio_VTable =
{
    Basicaudio_QueryInterface,
    Basicaudio_AddRef,
    Basicaudio_Release,
    Basicaudio_GetTypeInfoCount,
    Basicaudio_GetTypeInfo,
    Basicaudio_GetIDsOfNames,
    Basicaudio_Invoke,
    Basicaudio_put_Volume,
    Basicaudio_get_Volume,
    Basicaudio_put_Balance,
    Basicaudio_get_Balance
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Basicvideo_GetTypeInfoCount(IBasicVideo *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetTypeInfo(IBasicVideo *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d, %ld, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetIDsOfNames(IBasicVideo *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %ld, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_Invoke(IBasicVideo *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %s (%p), %ld, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI Basicvideo_get_AvgTimePerFrame(IBasicVideo *iface,
						     REFTIME *pAvgTimePerFrame) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pAvgTimePerFrame);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitRate(IBasicVideo *iface,
					     long *pBitRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pBitRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitErrorRate(IBasicVideo *iface,
						  long *pBitErrorRate) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pBitErrorRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoWidth(IBasicVideo *iface,
						long *pVideoWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pVideoWidth);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoHeight(IBasicVideo *iface,
						 long *pVideoHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pVideoHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceLeft(IBasicVideo *iface,
						long SourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, SourceLeft);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceLeft(IBasicVideo *iface,
						long *pSourceLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pSourceLeft);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceWidth(IBasicVideo *iface,
						 long SourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, SourceWidth);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceWidth(IBasicVideo *iface,
						 long *pSourceWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pSourceWidth);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceTop(IBasicVideo *iface,
					       long SourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, SourceTop);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceTop(IBasicVideo *iface,
					       long *pSourceTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pSourceTop);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceHeight(IBasicVideo *iface,
						  long SourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, SourceHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceHeight(IBasicVideo *iface,
						  long *pSourceHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pSourceHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationLeft(IBasicVideo *iface,
						     long DestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, DestinationLeft);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationLeft(IBasicVideo *iface,
						     long *pDestinationLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDestinationLeft);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationWidth(IBasicVideo *iface,
						      long DestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, DestinationWidth);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationWidth(IBasicVideo *iface,
						      long *pDestinationWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDestinationWidth);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationTop(IBasicVideo *iface,
						    long DestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, DestinationTop);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationTop(IBasicVideo *iface,
						    long *pDestinationTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDestinationTop);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationHeight(IBasicVideo *iface,
						       long DestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, DestinationHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationHeight(IBasicVideo *iface,
						       long *pDestinationHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pDestinationHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetSourcePosition(IBasicVideo *iface,
						   long Left,
						   long Top,
						   long Width,
						   long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld): stub !!!\n", This, iface, Left, Top, Width, Height);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetSourcePosition(IBasicVideo *iface,
						   long *pLeft,
						   long *pTop,
						   long *pWidth,
						   long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultSourcePosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDestinationPosition(IBasicVideo *iface,
							long Left,
							long Top,
							long Width,
							long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld): stub !!!\n", This, iface, Left, Top, Width, Height);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetDestinationPosition(IBasicVideo *iface,
							long *pLeft,
							long *pTop,
							long *pWidth,
							long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultDestinationPosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoSize(IBasicVideo *iface,
					      long *pWidth,
					      long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoPaletteEntries(IBasicVideo *iface,
							long StartIndex,
							long Entries,
							long *pRetrieved,
							long *pPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %p, %p): stub !!!\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetCurrentImage(IBasicVideo *iface,
						 long *pBufferSize,
						 long *pDIBImage) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pBufferSize, pDIBImage);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_IsUsingDefaultSource(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_IsUsingDefaultDestination(IBasicVideo *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static IBasicVideoVtbl IBasicVideo_VTable =
{
    Basicvideo_QueryInterface,
    Basicvideo_AddRef,
    Basicvideo_Release,
    Basicvideo_GetTypeInfoCount,
    Basicvideo_GetTypeInfo,
    Basicvideo_GetIDsOfNames,
    Basicvideo_Invoke,
    Basicvideo_get_AvgTimePerFrame,
    Basicvideo_get_BitRate,
    Basicvideo_get_BitErrorRate,
    Basicvideo_get_VideoWidth,
    Basicvideo_get_VideoHeight,
    Basicvideo_put_SourceLeft,
    Basicvideo_get_SourceLeft,
    Basicvideo_put_SourceWidth,
    Basicvideo_get_SourceWidth,
    Basicvideo_put_SourceTop,
    Basicvideo_get_SourceTop,
    Basicvideo_put_SourceHeight,
    Basicvideo_get_SourceHeight,
    Basicvideo_put_DestinationLeft,
    Basicvideo_get_DestinationLeft,
    Basicvideo_put_DestinationWidth,
    Basicvideo_get_DestinationWidth,
    Basicvideo_put_DestinationTop,
    Basicvideo_get_DestinationTop,
    Basicvideo_put_DestinationHeight,
    Basicvideo_get_DestinationHeight,
    Basicvideo_SetSourcePosition,
    Basicvideo_GetSourcePosition,
    Basicvideo_SetDefaultSourcePosition,
    Basicvideo_SetDestinationPosition,
    Basicvideo_GetDestinationPosition,
    Basicvideo_SetDefaultDestinationPosition,
    Basicvideo_GetVideoSize,
    Basicvideo_GetVideoPaletteEntries,
    Basicvideo_GetCurrentImage,
    Basicvideo_IsUsingDefaultSource,
    Basicvideo_IsUsingDefaultDestination
};


/*** IUnknown methods ***/
static HRESULT WINAPI Videowindow_QueryInterface(IVideoWindow *iface,
						 REFIID riid,
						 LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Videowindow_GetTypeInfoCount(IVideoWindow *iface,
						   UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetTypeInfo(IVideoWindow *iface,
					      UINT iTInfo,
					      LCID lcid,
					      ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d, %ld, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetIDsOfNames(IVideoWindow *iface,
						REFIID riid,
						LPOLESTR*rgszNames,
						UINT cNames,
						LCID lcid,
						DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %ld, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Videowindow_Invoke(IVideoWindow *iface,
					 DISPID dispIdMember,
					 REFIID riid,
					 LCID lcid,
					 WORD wFlags,
					 DISPPARAMS*pDispParams,
					 VARIANT*pVarResult,
					 EXCEPINFO*pExepInfo,
					 UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %s (%p), %ld, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IVideoWindow methods ***/
static HRESULT WINAPI Videowindow_put_Caption(IVideoWindow *iface,
					      BSTR strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p)): stub !!!\n", This, iface, debugstr_w(strCaption), strCaption);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, strCaption);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyle(IVideoWindow *iface,
						  long WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, WindowStyle);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyle(IVideoWindow *iface,
						  long *WindowStyle) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, WindowStyle);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyleEx(IVideoWindow *iface,
						    long WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, WindowStyleEx);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyleEx(IVideoWindow *iface,
						    long *WindowStyleEx) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, WindowStyleEx);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_AutoShow(IVideoWindow *iface,
					       long AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, AutoShow);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_AutoShow(IVideoWindow *iface,
					       long *AutoShow) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, AutoShow);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowState(IVideoWindow *iface,
						  long WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowState(IVideoWindow *iface,
						  long *WindowState) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BackgroundPalette(IVideoWindow *iface,
							long BackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BackgroundPalette(IVideoWindow *iface,
							long *pBackgroundPalette) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Visible(IVideoWindow *iface,
					      long Visible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Visible);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Visible(IVideoWindow *iface,
					      long *pVisible) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pVisible);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Left(IVideoWindow *iface,
					   long Left) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Left);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Left(IVideoWindow *iface,
					   long *pLeft) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pLeft);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Width(IVideoWindow *iface,
					    long Width) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Width);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Width(IVideoWindow *iface,
					    long *pWidth) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pWidth);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Top(IVideoWindow *iface,
					  long Top) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Top);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Top(IVideoWindow *iface,
					  long *pTop) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pTop);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Height(IVideoWindow *iface,
					     long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Height);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Height(IVideoWindow *iface,
					     long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08lx): stub !!!\n", This, iface, (DWORD) Owner);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08lx): stub !!!\n", This, iface, (DWORD) Owner);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_MessageDrain(IVideoWindow *iface,
						   OAHWND Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08lx): stub !!!\n", This, iface, (DWORD) Drain);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_MessageDrain(IVideoWindow *iface,
						   OAHWND *Drain) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, Drain);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BorderColor(IVideoWindow *iface,
						  long *Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BorderColor(IVideoWindow *iface,
						  long Color) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_FullScreenMode(IVideoWindow *iface,
						     long *FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_FullScreenMode(IVideoWindow *iface,
						     long FullScreenMode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowForeground(IVideoWindow *iface,
						      long Focus) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, Focus);

    return S_OK;
}

static HRESULT WINAPI Videowindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
						     long uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08lx, %ld, %08lx, %08lx): stub !!!\n", This, iface, (DWORD) hwnd, uMsg, wParam, lParam);

    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowPosition(IVideoWindow *iface,
						    long Left,
						    long Top,
						    long Width,
						    long Height) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);
    
    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld): stub !!!\n", This, iface, Left, Top, Width, Height);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetWindowPosition(IVideoWindow *iface,
						    long *pLeft,
						    long *pTop,
						    long *pWidth,
						    long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMinIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMaxIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetRestorePosition(IVideoWindow *iface,
						     long *pLeft,
						     long *pTop,
						     long *pWidth,
						     long *pHeight) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_HideCursor(IVideoWindow *iface,
					     long HideCursor) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

static HRESULT WINAPI Videowindow_IsCursorHidden(IVideoWindow *iface,
						 long *CursorHidden) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, CursorHidden);

    return S_OK;
}


static IVideoWindowVtbl IVideoWindow_VTable =
{
    Videowindow_QueryInterface,
    Videowindow_AddRef,
    Videowindow_Release,
    Videowindow_GetTypeInfoCount,
    Videowindow_GetTypeInfo,
    Videowindow_GetIDsOfNames,
    Videowindow_Invoke,
    Videowindow_put_Caption,
    Videowindow_get_Caption,
    Videowindow_put_WindowStyle,
    Videowindow_get_WindowStyle,
    Videowindow_put_WindowStyleEx,
    Videowindow_get_WindowStyleEx,
    Videowindow_put_AutoShow,
    Videowindow_get_AutoShow,
    Videowindow_put_WindowState,
    Videowindow_get_WindowState,
    Videowindow_put_BackgroundPalette,
    Videowindow_get_BackgroundPalette,
    Videowindow_put_Visible,
    Videowindow_get_Visible,
    Videowindow_put_Left,
    Videowindow_get_Left,
    Videowindow_put_Width,
    Videowindow_get_Width,
    Videowindow_put_Top,
    Videowindow_get_Top,
    Videowindow_put_Height,
    Videowindow_get_Height,
    Videowindow_put_Owner,
    Videowindow_get_Owner,
    Videowindow_put_MessageDrain,
    Videowindow_get_MessageDrain,
    Videowindow_get_BorderColor,
    Videowindow_put_BorderColor,
    Videowindow_get_FullScreenMode,
    Videowindow_put_FullScreenMode,
    Videowindow_SetWindowForeground,
    Videowindow_NotifyOwnerMessage,
    Videowindow_SetWindowPosition,
    Videowindow_GetWindowPosition,
    Videowindow_GetMinIdealImageSize,
    Videowindow_GetMaxIdealImageSize,
    Videowindow_GetRestorePosition,
    Videowindow_HideCursor,
    Videowindow_IsCursorHidden
};


/*** IUnknown methods ***/
static HRESULT WINAPI Mediaevent_QueryInterface(IMediaEventEx *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return Filtergraph_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI Mediaevent_AddRef(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI Mediaevent_Release(IMediaEventEx *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return Filtergraph_Release(This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Mediaevent_GetTypeInfoCount(IMediaEventEx *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetTypeInfo(IMediaEventEx *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%d, %ld, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetIDsOfNames(IMediaEventEx *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %ld, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_Invoke(IMediaEventEx *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %s (%p), %ld, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IMediaEvent methods ***/
static HRESULT WINAPI Mediaevent_GetEventHandle(IMediaEventEx *iface,
						OAEVENT *hEvent) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, hEvent);

    *hEvent = (OAEVENT)This->evqueue.msg_event;

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetEvent(IMediaEventEx *iface,
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

static HRESULT WINAPI Mediaevent_WaitForCompletion(IMediaEventEx *iface,
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

static HRESULT WINAPI Mediaevent_CancelDefaultHandling(IMediaEventEx *iface,
						       long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = FALSE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = FALSE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI Mediaevent_RestoreDefaultHandling(IMediaEventEx *iface,
							long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = TRUE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = TRUE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI Mediaevent_FreeEventParams(IMediaEventEx *iface,
						 long lEvCode,
						 LONG_PTR lParam1,
						 LONG_PTR lParam2) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %08lx, %08lx): stub !!!\n", This, iface, lEvCode, lParam1, lParam2);

    return S_OK;
}

/*** IMediaEventEx methods ***/
static HRESULT WINAPI Mediaevent_SetNotifyWindow(IMediaEventEx *iface,
						 OAHWND hwnd,
						 long lMsg,
						 LONG_PTR lInstanceData) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%08lx, %ld, %08lx)\n", This, iface, (DWORD) hwnd, lMsg, lInstanceData);

    This->notif.hWnd = (HWND)hwnd;
    This->notif.msg = lMsg;
    This->notif.instance = (long) lInstanceData;

    return S_OK;
}

static HRESULT WINAPI Mediaevent_SetNotifyFlags(IMediaEventEx *iface,
						long lNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, lNoNotifyFlags);

    if ((lNoNotifyFlags != 0) || (lNoNotifyFlags != 1))
	return E_INVALIDARG;

    This->notif.disabled = lNoNotifyFlags;

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetNotifyFlags(IMediaEventEx *iface,
						long *lplNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, lplNoNotifyFlags);

    if (!lplNoNotifyFlags)
	return E_POINTER;

    *lplNoNotifyFlags = This->notif.disabled;

    return S_OK;
}


static IMediaEventExVtbl IMediaEventEx_VTable =
{
    Mediaevent_QueryInterface,
    Mediaevent_AddRef,
    Mediaevent_Release,
    Mediaevent_GetTypeInfoCount,
    Mediaevent_GetTypeInfo,
    Mediaevent_GetIDsOfNames,
    Mediaevent_Invoke,
    Mediaevent_GetEventHandle,
    Mediaevent_GetEvent,
    Mediaevent_WaitForCompletion,
    Mediaevent_CancelDefaultHandling,
    Mediaevent_RestoreDefaultHandling,
    Mediaevent_FreeEventParams,
    Mediaevent_SetNotifyWindow,
    Mediaevent_SetNotifyFlags,
    Mediaevent_GetNotifyFlags
};


static HRESULT WINAPI MediaFilter_QueryInterface(IMediaFilter *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    return Filtergraph_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI MediaFilter_AddRef(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    return Filtergraph_AddRef(This);
}

static ULONG WINAPI MediaFilter_Release(IMediaFilter *iface)
{
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

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
    FIXME("(%lld): stub\n", tStart);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_GetState(IMediaFilter *iface, DWORD dwMsTimeout, FILTER_STATE * pState)
{
    FIXME("(%ld, %p): stub\n", dwMsTimeout, pState);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_SetSyncSource(IMediaFilter *iface, IReferenceClock *pClock)
{
    FIXME("(%p): stub\n", pClock);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_GetSyncSource(IMediaFilter *iface, IReferenceClock **ppClock)
{
    FIXME("(%p): stub\n", ppClock);

    return E_NOTIMPL;
}

static IMediaFilterVtbl IMediaFilter_VTable =
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

static IMediaEventSinkVtbl IMediaEventSink_VTable =
{
    MediaEventSink_QueryInterface,
    MediaEventSink_AddRef,
    MediaEventSink_Release,
    MediaEventSink_Notify
};

/* This is the only function that actually creates a FilterGraph class... */
HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IFilterGraphImpl *fimpl;
    HRESULT hr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    fimpl = HeapAlloc(GetProcessHeap(), 0, sizeof(*fimpl));
    fimpl->IGraphBuilder_vtbl = &IGraphBuilder_VTable;
    fimpl->IMediaControl_vtbl = &IMediaControl_VTable;
    fimpl->IMediaSeeking_vtbl = &IMediaSeeking_VTable;
    fimpl->IBasicAudio_vtbl = &IBasicAudio_VTable;
    fimpl->IBasicVideo_vtbl = &IBasicVideo_VTable;
    fimpl->IVideoWindow_vtbl = &IVideoWindow_VTable;
    fimpl->IMediaEventEx_vtbl = &IMediaEventEx_VTable;
    fimpl->IMediaFilter_vtbl = &IMediaFilter_VTable;
    fimpl->IMediaEventSink_vtbl = &IMediaEventSink_VTable;
    fimpl->ref = 1;
    fimpl->ppFiltersInGraph = NULL;
    fimpl->pFilterNames = NULL;
    fimpl->nFilters = 0;
    fimpl->filterCapacity = 0;
    fimpl->nameIndex = 1;
    fimpl->hEventCompletion = CreateEventW(0, TRUE, FALSE, 0);
    fimpl->HandleEcComplete = TRUE;
    fimpl->HandleEcRepaint = TRUE;
    fimpl->notif.hWnd = 0;
    fimpl->notif.disabled = FALSE;
    fimpl->nRenderers = 0;
    fimpl->EcCompleteCount = 0;
    fimpl->state = State_Stopped;
    EventsQueue_Init(&fimpl->evqueue);
    InitializeCriticalSection(&fimpl->cs);

    hr = CoCreateInstance(&CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&fimpl->pFilterMapper2);
    if (FAILED(hr)) {
        ERR("Unable to create filter mapper (%lx)\n", hr);
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
