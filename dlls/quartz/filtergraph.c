/*              DirectShow FilterGraph object (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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
#include "dshow.h"
#include "wine/debug.h"
#include "strmif.h"
#include "wine/unicode.h"

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct _IFilterGraphImpl {
    ICOM_VTABLE(IGraphBuilder) *IGraphBuilder_vtbl;
    ICOM_VTABLE(IMediaControl) *IMediaControl_vtbl;
    ICOM_VTABLE(IMediaSeeking) *IMediaSeeking_vtbl;
    ICOM_VTABLE(IBasicAudio) *IBasicAudio_vtbl;
    ICOM_VTABLE(IBasicVideo) *IBasicVideo_vtbl;
    ICOM_VTABLE(IVideoWindow) *IVideoWindow_vtbl;
    ICOM_VTABLE(IMediaEventEx) *IMediaEventEx_vtbl;
    ICOM_VTABLE(IMediaFilter) *IMediaFilter_vtbl;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IBasicVideo2 */
    /* IFilterChain */
    /* IFilterGraph2 */
    /* IFilterMapper2 */
    /* IGraphConfig */
    /* IGraphVersion */
    /* IMediaEventSink */
    /* IMediaPosition */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceMananger */
    /* IServiceProvider */
    /* IVideoFrameStep */

    ULONG ref;
    IBaseFilter ** ppFiltersInGraph;
    LPWSTR * pFilterNames;
    int nFilters;
    int filterCapacity;
} IFilterGraphImpl;


static HRESULT Filtergraph_QueryInterface(IFilterGraphImpl *This,
					  REFIID riid,
					  LPVOID *ppvObj) {
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);
    
    if (IsEqualGUID(&IID_IUnknown, riid) ||
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
    } else {
        *ppvObj = NULL;
	FIXME("unknown interface %s\n", debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    This->ref++;
    return S_OK;
}

static ULONG Filtergraph_AddRef(IFilterGraphImpl *This) {
    TRACE("(%p)->(): new ref = %ld\n", This, This->ref + 1);
    
    return ++This->ref;
}

static ULONG Filtergraph_Release(IFilterGraphImpl *This) {
    static ULONG ref;
    
    TRACE("(%p)->(): new ref = %ld\n", This, This->ref - 1);
    
    ref = --This->ref;
    if (ref == 0) {
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
    int size;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %s (%p))\n", This, iface, pFilter, debugstr_w(pName), pName);

    /* FIXME: generate name of filter if NULL */

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

    hr = IBaseFilter_JoinFilterGraph(pFilter, (IFilterGraph *)This, pName);

    if (SUCCEEDED(hr))
    {
        size = (strlenW(pName) + 1) * sizeof(WCHAR);
        IBaseFilter_AddRef(pFilter);
        This->ppFiltersInGraph[This->nFilters] = pFilter;
        This->pFilterNames[This->nFilters] = CoTaskMemAlloc(size);
        memcpy(This->pFilterNames[This->nFilters], pName, size);
        This->nFilters++;
    }

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

static HRESULT WINAPI Graphbuilder_EnumFilter(IGraphBuilder *iface,
					      IEnumFilters **ppEnum) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppEnum);

    return S_OK;
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

static HRESULT WINAPI Graphbuilder_ConnectDirect(IGraphBuilder *iface,
						 IPin *ppinIn,
						 IPin *ppinOut,
						 const AM_MEDIA_TYPE *pmt) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);

    return IPin_Connect(ppinOut, ppinIn, pmt);
}

static HRESULT WINAPI Graphbuilder_Reconnect(IGraphBuilder *iface,
					     IPin *ppin) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppin);

    return S_OK;
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

/*** IGraphBuilder methods ***/
static HRESULT WINAPI Graphbuilder_Connect(IGraphBuilder *iface,
					   IPin *ppinOut,
					   IPin *ppinIn) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p): stub !!!\n", This, iface, ppinOut, ppinIn);

    return S_OK;
}

static HRESULT WINAPI Graphbuilder_Render(IGraphBuilder *iface,
					  IPin *ppinOut) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, ppinOut);

    return S_OK;
}

static HRESULT WINAPI Graphbuilder_RenderFile(IGraphBuilder *iface,
					      LPCWSTR lpcwstrFile,
					      LPCWSTR lpcwstrPlayList) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %s (%p)): stub !!!\n", This, iface, debugstr_w(lpcwstrFile), lpcwstrFile, debugstr_w(lpcwstrPlayList), lpcwstrPlayList);

    return S_OK;
}

static HRESULT WINAPI Graphbuilder_AddSourceFilter(IGraphBuilder *iface,
						   LPCWSTR lpcwstrFileName,
						   LPCWSTR lpcwstrFilterName,
						   IBaseFilter **ppFilter) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IGraphBuilder_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %s (%p), %p): stub !!!\n", This, iface, debugstr_w(lpcwstrFileName), lpcwstrFileName, debugstr_w(lpcwstrFilterName), lpcwstrFilterName, ppFilter);

    return S_OK;
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


static ICOM_VTABLE(IGraphBuilder) IGraphBuilder_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    Graphbuilder_QueryInterface,
    Graphbuilder_AddRef,
    Graphbuilder_Release,
    Graphbuilder_AddFilter,
    Graphbuilder_RemoveFilter,
    Graphbuilder_EnumFilter,
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

/*** IMediaControl methods ***/
static HRESULT WINAPI Mediacontrol_Run(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_Pause(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_Stop(IMediaControl *iface) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Mediacontrol_GetState(IMediaControl *iface,
					    LONG msTimeout,
					    OAFilterState *pfs) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaControl_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %p): stub !!!\n", This, iface, msTimeout, pfs);

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


static ICOM_VTABLE(IMediaControl) IMediaControl_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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


static ICOM_VTABLE(IMediaSeeking) IMediaSeeking_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static ICOM_VTABLE(IBasicAudio) IBasicAudio_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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


static ICOM_VTABLE(IBasicVideo) IBasicVideo_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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


static ICOM_VTABLE(IVideoWindow) IVideoWindow_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, hEvent);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetEvent(IMediaEventEx *iface,
					  long *lEventCode,
					  LONG_PTR *lParam1,
					  LONG_PTR *lParam2,
					  long msTimeout) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %ld): stub !!!\n", This, iface, lEventCode, lParam1, lParam2, msTimeout);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_WaitForCompletion(IMediaEventEx *iface,
						   long msTimeout,
						   long *pEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %p): stub !!!\n", This, iface, msTimeout, pEvCode);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_CancelDefaultHandling(IMediaEventEx *iface,
						       long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, lEvCode);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_RestoreDefaultHandling(IMediaEventEx *iface,
							long lEvCode) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, lEvCode);

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

    TRACE("(%p/%p)->(%08lx, %ld, %08lx): stub !!!\n", This, iface, (DWORD) hwnd, lMsg, lInstanceData);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_SetNotifyFlags(IMediaEventEx *iface,
						long lNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%ld): stub !!!\n", This, iface, lNoNotifyFlags);

    return S_OK;
}

static HRESULT WINAPI Mediaevent_GetNotifyFlags(IMediaEventEx *iface,
						long *lplNoNotifyFlags) {
    ICOM_THIS_MULTI(IFilterGraphImpl, IMediaEventEx_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, lplNoNotifyFlags);

    return S_OK;
}


static ICOM_VTABLE(IMediaEventEx) IMediaEventEx_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static ICOM_VTABLE(IMediaFilter) IMediaFilter_VTable =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

/* This is the only function that actually creates a FilterGraph class... */
HRESULT FILTERGRAPH_create(IUnknown *pUnkOuter, LPVOID *ppObj) {
    IFilterGraphImpl *fimpl;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fimpl = (IFilterGraphImpl *) HeapAlloc(GetProcessHeap(), 0, sizeof(*fimpl));
    fimpl->IGraphBuilder_vtbl = &IGraphBuilder_VTable;
    fimpl->IMediaControl_vtbl = &IMediaControl_VTable;
    fimpl->IMediaSeeking_vtbl = &IMediaSeeking_VTable;
    fimpl->IBasicAudio_vtbl = &IBasicAudio_VTable;
    fimpl->IBasicVideo_vtbl = &IBasicVideo_VTable;
    fimpl->IVideoWindow_vtbl = &IVideoWindow_VTable;
    fimpl->IMediaEventEx_vtbl = &IMediaEventEx_VTable;
    fimpl->IMediaFilter_vtbl = &IMediaFilter_VTable;
    fimpl->ref = 1;
    fimpl->ppFiltersInGraph = NULL;
    fimpl->pFilterNames = NULL;
    fimpl->nFilters = 0;
    fimpl->filterCapacity = 0;

    *ppObj = fimpl;
    return S_OK;
}
