/*
 * Copyright (C) 2002 Lionel Ulmer
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

#ifndef __STRMIF_INCLUDED__
#define __STRMIF_INCLUDED__

#include "windef.h"
#include "wingdi.h"
#include "objbase.h"

/* FilterGraph object / interface */
typedef struct IFilterGraph IFilterGraph;
typedef struct IBaseFilter IBaseFilter;
typedef struct IEnumFilters IEnumFilters;
typedef struct IPin IPin;
typedef struct IEnumPins IEnumPins;
typedef struct IEnumMediaTypes IEnumMediaTypes;
typedef struct IMediaFilter IMediaFilter;
typedef struct IReferenceClock IReferenceClock;
typedef struct IGraphBuilder IGraphBuilder;
typedef struct IMediaSeeking IMediaSeeking;

typedef LONGLONG REFERENCE_TIME;
typedef double REFTIME;

typedef struct  _MediaType {
    GUID      majortype;
    GUID      subtype;
    BOOL      bFixedSizeSamples;
    BOOL      bTemporalCompression;
    ULONG     lSampleSize;
    GUID      formattype;
    IUnknown  *pUnk;
    ULONG     cbFormat;
    /* [size_is] */ BYTE *pbFormat;
} AM_MEDIA_TYPE;

#define MAX_FILTER_NAME  128
typedef struct _FilterInfo {
    WCHAR        achName[MAX_FILTER_NAME]; 
    IFilterGraph *pGraph;
} FILTER_INFO;

typedef enum _FilterState {
    State_Stopped   = 0,
    State_Paused    = 1,
    State_Running   = 2
} FILTER_STATE;

typedef enum _PinDirection  {
    PINDIR_INPUT  = 0,
    PINDIR_OUTPUT = 1
} PIN_DIRECTION;

#define MAX_PIN_NAME 128
typedef struct _PinInfo {
    IBaseFilter *pFilter;
    PIN_DIRECTION dir;
    WCHAR achName[MAX_PIN_NAME];
} PIN_INFO;

typedef DWORD_PTR HSEMAPHORE;
typedef DWORD_PTR HEVENT;

#define INTERFACE IPin
#define IPin_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Connect)(THIS_  IPin *  pReceivePin, AM_MEDIA_TYPE *  pmt) PURE; \
    STDMETHOD(ReceiveConnection)(THIS_  IPin *  pConnector, AM_MEDIA_TYPE *  pmt) PURE; \
    STDMETHOD(Disconnect)(THIS) PURE; \
    STDMETHOD(ConnectedTo)(THIS_  IPin **  pPin) PURE; \
    STDMETHOD(ConnectionMediaType)(THIS_  AM_MEDIA_TYPE *  pmt) PURE; \
    STDMETHOD(QueryPinInfo)(THIS_  PIN_INFO *  pInfo) PURE; \
    STDMETHOD(QueryDirection)(THIS_  PIN_DIRECTION *  pPinDir) PURE; \
    STDMETHOD(QueryId)(THIS_  LPWSTR *  Id) PURE; \
    STDMETHOD(QueryAccept)(THIS_  AM_MEDIA_TYPE *  pmt) PURE; \
    STDMETHOD(EnumMediaTypes)(THIS_  IEnumMediaTypes **  ppEnum) PURE; \
    STDMETHOD(QueryInternalConnections)(THIS_  IPin **  apPin, ULONG *  nPin) PURE; \
    STDMETHOD(EndOfStream)(THIS) PURE; \
    STDMETHOD(BeginFlush)(THIS) PURE; \
    STDMETHOD(EndFlush)(THIS) PURE; \
    STDMETHOD(NewSegment)(THIS_  REFERENCE_TIME   tStart, REFERENCE_TIME   tStop, double   dRate) PURE;
ICOM_DEFINE(IPin,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPin_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IPin_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IPin_Release(p)   (p)->lpVtbl->Release(p)
/*** IPin methods ***/
#define IPin_Connect(p,a,b)   (p)->lpVtbl->Connect(p,a,b)
#define IPin_ReceiveConnection(p,a,b)   (p)->lpVtbl->ReceiveConnection(p,a,b)
#define IPin_Disconnect(p)   (p)->lpVtbl->Disconnect(p)
#define IPin_ConnectedTo(p,a)   (p)->lpVtbl->ConnectedTo(p,a)
#define IPin_ConnectionMediaType(p,a)   (p)->lpVtbl->ConnectionMediaType(p,a)
#define IPin_QueryPinInfo(p,a)   (p)->lpVtbl->QueryPinInfo(p,a)
#define IPin_QueryDirection(p,a)   (p)->lpVtbl->QueryDirection(p,a)
#define IPin_QueryId(p,a)   (p)->lpVtbl->QueryId(p,a)
#define IPin_QueryAccept(p,a)   (p)->lpVtbl->QueryAccept(p,a)
#define IPin_EnumMediaTypes(p,a)   (p)->lpVtbl->EnumMediaTypes(p,a)
#define IPin_QueryInternalConnections(p,a,b)   (p)->lpVtbl->QueryInternalConnections(p,a,b)
#define IPin_EndOfStream(p)   (p)->lpVtbl->EndOfStream(p)
#define IPin_BeginFlush(p)   (p)->lpVtbl->BeginFlush(p)
#define IPin_EndFlush(p)   (p)->lpVtbl->EndFlush(p)
#define IPin_NewSegment(p,a,b,c)   (p)->lpVtbl->NewSegment(p,a,b,c)
#endif

#define INTERFACE IEnumPins
#define IEnumPins_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Next)(THIS_  ULONG   cPins, IPin **  ppPins, ULONG *  pcFetched) PURE; \
    STDMETHOD(Skip)(THIS_  ULONG   cPins) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_  IEnumPins **  ppEnum) PURE;
ICOM_DEFINE(IEnumPins,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumPins_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumPins_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IEnumPins_Release(p)   (p)->lpVtbl->Release(p)
/*** IEnumPins methods ***/
#define IEnumPins_Next(p,a,b,c)   (p)->lpVtbl->Next(p,a,b,c)
#define IEnumPins_Skip(p,a)   (p)->lpVtbl->Skip(p,a)
#define IEnumPins_Reset(p)   (p)->lpVtbl->Reset(p)
#define IEnumPins_Clone(p,a)   (p)->lpVtbl->Clone(p,a)
#endif

#define INTERFACE IMediaFilter
#define IMediaFilter_METHODS \
    IPersist_METHODS \
    STDMETHOD(Stop)(THIS) PURE; \
    STDMETHOD(Pause)(THIS) PURE; \
    STDMETHOD(Run)(THIS_  REFERENCE_TIME  tStart) PURE; \
    STDMETHOD(GetState)(THIS_  DWORD   dwMilliSecsTimeout, FILTER_STATE *  State) PURE; \
    STDMETHOD(SetSyncSource)(THIS_  IReferenceClock *  pClock) PURE; \
    STDMETHOD(GetSyncSource)(THIS_  IReferenceClock **  pClock) PURE;
ICOM_DEFINE(IMediaFilter,IPersist)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMediaFilter_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IMediaFilter_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IMediaFilter_Release(p)   (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IMediaFilter_GetClassID(p,a)   (p)->lpVtbl->GetClassID(p,a)
/*** IMediaFilter methods ***/
#define IMediaFilter_Stop(p)   (p)->lpVtbl->Stop(p)
#define IMediaFilter_Pause(p)   (p)->lpVtbl->Pause(p)
#define IMediaFilter_Run(p,a)   (p)->lpVtbl->Run(p,a)
#define IMediaFilter_GetState(p,a,b)   (p)->lpVtbl->GetState(p,a,b)
#define IMediaFilter_SetSyncSource(p,a)   (p)->lpVtbl->SetSyncSource(p,a)
#define IMediaFilter_GetSyncSource(p,a)   (p)->lpVtbl->GetSyncSource(p,a)
#endif

#define INTERFACE IBaseFilter
#define IBaseFilter_METHODS \
    IMediaFilter_METHODS \
    STDMETHOD(EnumPins)(THIS_  IEnumPins **  ppEnum) PURE; \
    STDMETHOD(FindPin)(THIS_  LPCWSTR   Id, IPin **  ppPin) PURE; \
    STDMETHOD(QueryFilterInfo)(THIS_  FILTER_INFO *  pInfo) PURE; \
    STDMETHOD(JoinFilterGraph)(THIS_  IFilterGraph *  pGraph, LPCWSTR   pName) PURE; \
    STDMETHOD(QueryVendorInfo)(THIS_  LPWSTR *  pVendorInfo) PURE;
ICOM_DEFINE(IBaseFilter,IMediaFilter)
#undef INTERFACE
     
#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBaseFilter_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IBaseFilter_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IBaseFilter_Release(p)   (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IBaseFilter_GetClassID(p,a)   (p)->lpVtbl->GetClassID(p,a)
/*** IMediaFilter methods ***/
#define IBaseFilter_Stop(p)   (p)->lpVtbl->Stop(p)
#define IBaseFilter_Pause(p)   (p)->lpVtbl->Pause(p)
#define IBaseFilter_Run(p,a)   (p)->lpVtbl->Run(p,a)
#define IBaseFilter_GetState(p,a,b)   (p)->lpVtbl->GetState(p,a,b)
#define IBaseFilter_SetSyncSource(p,a)   (p)->lpVtbl->SetSyncSource(p,a)
#define IBaseFilter_GetSyncSource(p,a)   (p)->lpVtbl->GetSyncSource(p,a)
/*** IBaseFilter methods ***/
#define IBaseFilter_EnumPins(p,a)   (p)->lpVtbl->EnumPins(p,a)
#define IBaseFilter_FindPin(p,a,b)   (p)->lpVtbl->FindPin(p,a,b)
#define IBaseFilter_QueryFilterInfo(p,a)   (p)->lpVtbl->QueryFilterInfo(p,a)
#define IBaseFilter_JoinFilterGraph(p,a,b)   (p)->lpVtbl->JoinFilterGraph(p,a,b)
#define IBaseFilter_QueryVendorInfo(p,a)   (p)->lpVtbl->QueryVendorInfo(p,a)
#endif

#define INTERFACE IFilterGraph
#define IFilterGraph_METHODS \
    IUnknown_METHODS \
    STDMETHOD(AddFilter)(THIS_  IBaseFilter *  pFilter, LPCWSTR  pName) PURE; \
    STDMETHOD(RemoveFilter)(THIS_  IBaseFilter *  pFilter) PURE; \
    STDMETHOD(EnumFilter)(THIS_  IEnumFilters **  ppEnum) PURE; \
    STDMETHOD(FindFilterByName)(THIS_  LPCWSTR  pName, IBaseFilter **  ppFilter) PURE; \
    STDMETHOD(ConnectDirect)(THIS_  IPin *  ppinIn, IPin *  ppinOut, const AM_MEDIA_TYPE *  pmt) PURE; \
    STDMETHOD(Reconnect)(THIS_  IPin *  ppin) PURE; \
    STDMETHOD(Disconnect)(THIS_  IPin *  ppin) PURE; \
    STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
ICOM_DEFINE(IFilterGraph,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IFilterGraph_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IFilterGraph_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IFilterGraph_Release(p)   (p)->lpVtbl->Release(p)
/*** IFilterGraph methods ***/
#define IFilterGraph_AddFilter(p,a,b)   (p)->lpVtbl->AddFilter(p,a,b)
#define IFilterGraph_RemoveFilter(p,a)   (p)->lpVtbl->RemoveFilter(p,a)
#define IFilterGraph_EnumFilter(p,a)   (p)->lpVtbl->EnumFilter(p,a)
#define IFilterGraph_FindFilterByName(p,a,b)   (p)->lpVtbl->FindFilterByName(p,a,b)
#define IFilterGraph_ConnectDirect(p,a,b,c)   (p)->lpVtbl->ConnectDirect(p,a,b,c)
#define IFilterGraph_Reconnect(p,a)   (p)->lpVtbl->Reconnect(p,a)
#define IFilterGraph_Disconnect(p,a)   (p)->lpVtbl->Disconnect(p,a)
#define IFilterGraph_SetDefaultSyncSource(p)   (p)->lpVtbl->SetDefaultSyncSource(p)
#endif

#define INTERFACE IEnumFilters
#define IEnumFilters_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Next)(THIS_  ULONG   cFilters, IBaseFilter **  ppFilter, ULONG *  pcFetched) PURE; \
    STDMETHOD(Skip)(THIS_  ULONG   cFilters) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_  IEnumFilters **  ppEnum) PURE;
ICOM_DEFINE(IEnumFilters,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumFilters_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumFilters_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IEnumFilters_Release(p)   (p)->lpVtbl->Release(p)
/*** IEnumFilters methods ***/
#define IEnumFilters_Next(p,a,b,c)   (p)->lpVtbl->Next(p,a,b,c)
#define IEnumFilters_Skip(p,a)   (p)->lpVtbl->Skip(p,a)
#define IEnumFilters_Reset(p)   (p)->lpVtbl->Reset(p)
#define IEnumFilters_Clone(p,a)   (p)->lpVtbl->Clone(p,a)
#endif
    
#define INTERFACE IEnumMediaTypes
#define IEnumMediaTypes_METHODS \
    IUnknown_METHODS \
    STDMETHOD(Next)(THIS_  ULONG   cMediaTypes, AM_MEDIA_TYPE **  ppMediaTypes, ULONG *  pcFetched) PURE; \
    STDMETHOD(Skip)(THIS_  ULONG   cMediaTypes) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_  IEnumMediaTypes **  ppEnum) PURE;
ICOM_DEFINE(IEnumMediaTypes,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumMediaTypes_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumMediaTypes_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IEnumMediaTypes_Release(p)   (p)->lpVtbl->Release(p)
/*** IEnumMediaTypes methods ***/
#define IEnumMediaTypes_Next(p,a,b,c)   (p)->lpVtbl->Next(p,a,b,c)
#define IEnumMediaTypes_Skip(p,a)   (p)->lpVtbl->Skip(p,a)
#define IEnumMediaTypes_Reset(p)   (p)->lpVtbl->Reset(p)
#define IEnumMediaTypes_Clone(p,a)   (p)->lpVtbl->Clone(p,a)
#endif
     
#define INTERFACE IReferenceClock
#define IReferenceClock_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetTime)(THIS_  REFERENCE_TIME *  pTime) PURE; \
    STDMETHOD(AdviseTime)(THIS_  REFERENCE_TIME   baseTime, REFERENCE_TIME   streamTime, HEVENT   hEvent, DWORD_PTR *  pdwAdviseCookie) PURE; \
    STDMETHOD(AdvisePeriodic)(THIS_  REFERENCE_TIME   startTime, REFERENCE_TIME   periodTime, HSEMAPHORE   hSemaphore, DWORD_PTR *  pdwAdviseCookie) PURE; \
    STDMETHOD(Unadvise)(THIS_  DWORD_PTR   dwAdviseCookie) PURE;
ICOM_DEFINE(IReferenceClock,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IReferenceClock_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IReferenceClock_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IReferenceClock_Release(p)   (p)->lpVtbl->Release(p)
/*** IReferenceClock methods ***/
#define IReferenceClock_GetTime(p,a)   (p)->lpVtbl->GetTime(p,a)
#define IReferenceClock_AdviseTime(p,a,b,c,d)   (p)->lpVtbl->AdviseTime(p,a,b,c,d)
#define IReferenceClock_AdvisePeriodic(p,a,b,c,d)   (p)->lpVtbl->AdvisePeriodic(p,a,b,c,d)
#define IReferenceClock_Unadvise(p,a)   (p)->lpVtbl->Unadvise(p,a)
#endif
     
#define INTERFACE IGraphBuilder
#define IGraphBuilder_METHODS \
    IFilterGraph_METHODS \
    STDMETHOD(Connect)(THIS_  IPin *  ppinOut, IPin *  ppinIn) PURE; \
    STDMETHOD(Render)(THIS_  IPin *  ppinOut) PURE; \
    STDMETHOD(RenderFile)(THIS_  LPCWSTR   lpcwstrFile, LPCWSTR   lpcwstrPlayList) PURE; \
    STDMETHOD(AddSourceFilter)(THIS_  LPCWSTR   lpcwstrFileName, LPCWSTR   lpcwstrFilterName, IBaseFilter **  ppFilter) PURE; \
    STDMETHOD(SetLogFile)(THIS_  DWORD_PTR   hFile) PURE; \
    STDMETHOD(Abort)(THIS) PURE; \
    STDMETHOD(ShouldOperationContinue)(THIS) PURE;
ICOM_DEFINE(IGraphBuilder,IFilterGraph)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IGraphBuilder_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IGraphBuilder_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IGraphBuilder_Release(p)   (p)->lpVtbl->Release(p)
/*** IFilterGraph methods ***/
#define IGraphBuilder_AddFilter(p,a,b)   (p)->lpVtbl->AddFilter(p,a,b)
#define IGraphBuilder_RemoveFilter(p,a)   (p)->lpVtbl->RemoveFilter(p,a)
#define IGraphBuilder_EnumFilter(p,a)   (p)->lpVtbl->EnumFilter(p,a)
#define IGraphBuilder_FindFilterByName(p,a,b)   (p)->lpVtbl->FindFilterByName(p,a,b)
#define IGraphBuilder_ConnectDirect(p,a,b,c)   (p)->lpVtbl->ConnectDirect(p,a,b,c)
#define IGraphBuilder_Reconnect(p,a)   (p)->lpVtbl->Reconnect(p,a)
#define IGraphBuilder_Disconnect(p,a)   (p)->lpVtbl->Disconnect(p,a)
#define IGraphBuilder_SetDefaultSyncSource(p)   (p)->lpVtbl->SetDefaultSyncSource(p)
/*** IGraphBuilder methods ***/
#define IGraphBuilder_Connect(p,a,b)   (p)->lpVtbl->Connect(p,a,b)
#define IGraphBuilder_Render(p,a)   (p)->lpVtbl->Render(p,a)
#define IGraphBuilder_RenderFile(p,a,b)   (p)->lpVtbl->RenderFile(p,a,b)
#define IGraphBuilder_AddSourceFilter(p,a,b,c)   (p)->lpVtbl->AddSourceFilter(p,a,b,c)
#define IGraphBuilder_SetLogFile(p,a)   (p)->lpVtbl->SetLogFile(p,a)
#define IGraphBuilder_Abort(p)   (p)->lpVtbl->Abort(p)
#define IGraphBuilder_ShouldOperationContinue(p)   (p)->lpVtbl->ShouldOperationContinue(p)
#endif

#define INTERFACE IMediaSeeking
#define IMediaSeeking_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetCapabilities)(THIS_  DWORD *  pCapabilities) PURE; \
    STDMETHOD(CheckCapabilities)(THIS_  DWORD *  pCapabilities) PURE; \
    STDMETHOD(IsFormatSupported)(THIS_  GUID *  pFormat) PURE; \
    STDMETHOD(QueryPreferredFormat)(THIS_  GUID *  pFormat) PURE; \
    STDMETHOD(GetTimeFormat)(THIS_  GUID *  pFormat) PURE; \
    STDMETHOD(IsUsingTimeFormat)(THIS_  GUID *  pFormat) PURE; \
    STDMETHOD(SetTimeFormat)(THIS_  GUID *  pFormat) PURE; \
    STDMETHOD(GetDuration)(THIS_  LONGLONG *  pDuration) PURE; \
    STDMETHOD(GetStopPosition)(THIS_  LONGLONG *  pStop) PURE; \
    STDMETHOD(GetCurrentPosition)(THIS_  LONGLONG *  pCurrent) PURE; \
    STDMETHOD(ConvertTimeFormat)(THIS_  LONGLONG *  pTarget, GUID *  pTargetFormat, LONGLONG   Source, GUID *  pSourceFormat) PURE; \
    STDMETHOD(SetPositions)(THIS_  LONGLONG *  pCurrent, DWORD   dwCurrentFlags, LONGLONG *  pStop, DWORD   dwStopFlags) PURE; \
    STDMETHOD(GetPositions)(THIS_  LONGLONG *  pCurrent, LONGLONG *  pStop) PURE; \
    STDMETHOD(GetAvailable)(THIS_  LONGLONG *  pEarliest, LONGLONG *  pLatest) PURE; \
    STDMETHOD(SetRate)(THIS_  double   dRate) PURE; \
    STDMETHOD(GetRate)(THIS_  double *  pdRate) PURE; \
    STDMETHOD(GetPreroll)(THIS_  LONGLONG *  pllPreroll) PURE;
ICOM_DEFINE(IMediaSeeking,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMediaSeeking_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IMediaSeeking_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IMediaSeeking_Release(p)   (p)->lpVtbl->Release(p)
/*** IMediaSeeking methods ***/
#define IMediaSeeking_GetCapabilities(p,a)   (p)->lpVtbl->GetCapabilities(p,a)
#define IMediaSeeking_CheckCapabilities(p,a)   (p)->lpVtbl->CheckCapabilities(p,a)
#define IMediaSeeking_IsFormatSupported(p,a)   (p)->lpVtbl->IsFormatSupported(p,a)
#define IMediaSeeking_QueryPreferredFormat(p,a)   (p)->lpVtbl->QueryPreferredFormat(p,a)
#define IMediaSeeking_GetTimeFormat(p,a)   (p)->lpVtbl->GetTimeFormat(p,a)
#define IMediaSeeking_IsUsingTimeFormat(p,a)   (p)->lpVtbl->IsUsingTimeFormat(p,a)
#define IMediaSeeking_SetTimeFormat(p,a)   (p)->lpVtbl->SetTimeFormat(p,a)
#define IMediaSeeking_GetDuration(p,a)   (p)->lpVtbl->GetDuration(p,a)
#define IMediaSeeking_GetStopPosition(p,a)   (p)->lpVtbl->GetStopPosition(p,a)
#define IMediaSeeking_GetCurrentPosition(p,a)   (p)->lpVtbl->GetCurrentPosition(p,a)
#define IMediaSeeking_ConvertTimeFormat(p,a,b,c,d)   (p)->lpVtbl->ConvertTimeFormat(p,a,b,c,d)
#define IMediaSeeking_SetPositions(p,a,b,c,d)   (p)->lpVtbl->SetPositions(p,a,b,c,d)
#define IMediaSeeking_GetPositions(p,a,b)   (p)->lpVtbl->GetPositions(p,a,b)
#define IMediaSeeking_GetAvailable(p,a,b)   (p)->lpVtbl->GetAvailable(p,a,b)
#define IMediaSeeking_SetRate(p,a)   (p)->lpVtbl->SetRate(p,a)
#define IMediaSeeking_GetRate(p,a)   (p)->lpVtbl->GetRate(p,a)
#define IMediaSeeking_GetPreroll(p,a)   (p)->lpVtbl->GetPreroll(p,a)
#endif
     
#endif /* __STRMIF_INCLUDED__ */
