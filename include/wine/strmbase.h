/*
 * Header file for Wine's strmbase implementation
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

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);
void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType);

/* Pin functions */

typedef struct BasePin
{
	const struct IPinVtbl * lpVtbl;
	LONG refCount;
	LPCRITICAL_SECTION pCritSec;
	PIN_INFO pinInfo;
	IPin * pConnectedTo;
	AM_MEDIA_TYPE mtCurrent;
	REFERENCE_TIME tStart;
	REFERENCE_TIME tStop;
	double dRate;

	const struct BasePinFuncTable* pFuncsTable;
} BasePin;

typedef HRESULT (WINAPI *BasePin_CheckMediaType)(BasePin *This, const AM_MEDIA_TYPE *pmt);
typedef HRESULT (WINAPI *BasePin_AttemptConnection)(BasePin *This, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
typedef LONG (WINAPI *BasePin_GetMediaTypeVersion)(BasePin *This);
typedef HRESULT (WINAPI *BasePin_GetMediaType)(BasePin *This, int iPosition, AM_MEDIA_TYPE *amt);

typedef struct BasePinFuncTable {
	/* Required for Input Pins*/
	BasePin_CheckMediaType pfnCheckMediaType;
	/* Required for Output Pins*/
	BasePin_AttemptConnection pfnAttemptConnection;
	/* Required for BasePinImpl_EnumMediaTypes */
	BasePin_GetMediaTypeVersion pfnGetMediaTypeVersion;
	BasePin_GetMediaType pfnGetMediaType;
} BasePinFuncTable;

typedef struct BaseOutputPin
{
	/* inheritance C style! */
	BasePin pin;
	IMemInputPin * pMemInputPin;

	const struct BaseOutputPinFuncTable* pFuncsTable;
} BaseOutputPin;

typedef HRESULT (WINAPI *BaseOutputPin_DecideBufferSize)(BaseOutputPin *This, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
typedef HRESULT (WINAPI *BaseOutputPin_DecideAllocator)(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc);
typedef HRESULT (WINAPI *BaseOutputPin_BreakConnect)(BaseOutputPin * This);

typedef struct BaseOutputPinFuncTable {
	/* Required for BaseOutputPinImpl_DecideAllocator */
	BaseOutputPin_DecideBufferSize pfnDecideBufferSize;
	/* Required for BaseOutputPinImpl_AttemptConnection */
	BaseOutputPin_DecideAllocator pfnDecideAllocator;
	BaseOutputPin_BreakConnect pfnBreakConnect;
} BaseOutputPinFuncTable;

typedef struct BaseInputPin
{
	/* inheritance C style! */
	BasePin pin;

	const IMemInputPinVtbl * lpVtblMemInput;
	IMemAllocator * pAllocator;
	BOOL flushing, end_of_stream;
	IMemAllocator *preferred_allocator;

	const struct BaseInputPinFuncTable* pFuncsTable;
} BaseInputPin;

typedef HRESULT (WINAPI *BaseInputPin_Receive)(BaseInputPin *This, IMediaSample *pSample);

typedef struct BaseInputPinFuncTable {
	/* Optional */
	BaseInputPin_Receive pfnReceive;
} BaseInputPinFuncTable;

/* Base Pin */
HRESULT WINAPI BasePinImpl_GetMediaType(BasePin *This, int iPosition, AM_MEDIA_TYPE *pmt);
LONG WINAPI BasePinImpl_GetMediaTypeVersion(BasePin *This);
ULONG WINAPI BasePinImpl_AddRef(IPin * iface);
HRESULT WINAPI BasePinImpl_Disconnect(IPin * iface);
HRESULT WINAPI BasePinImpl_ConnectedTo(IPin * iface, IPin ** ppPin);
HRESULT WINAPI BasePinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo);
HRESULT WINAPI BasePinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir);
HRESULT WINAPI BasePinImpl_QueryId(IPin * iface, LPWSTR * Id);
HRESULT WINAPI BasePinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum);
HRESULT WINAPI BasePinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin);
HRESULT WINAPI BasePinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

/* Base Output Pin */
HRESULT WINAPI BaseOutputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI BaseOutputPinImpl_Release(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseOutputPinImpl_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseOutputPinImpl_Disconnect(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_EndOfStream(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_BeginFlush(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_EndFlush(IPin * iface);

HRESULT WINAPI BaseOutputPinImpl_GetDeliveryBuffer(BaseOutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags);
HRESULT WINAPI BaseOutputPinImpl_Deliver(BaseOutputPin * This, IMediaSample * pSample);
HRESULT WINAPI BaseOutputPinImpl_BreakConnect(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_Active(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_Inactive(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_InitAllocator(BaseOutputPin *This, IMemAllocator **pMemAlloc);
HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc);
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(BasePin *This, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);

HRESULT WINAPI BaseOutputPin_Construct(const IPinVtbl *OutputPin_Vtbl, LONG outputpin_size, const PIN_INFO * pPinInfo, const BasePinFuncTable* pBaseFuncsTable, const BaseOutputPinFuncTable* pBaseOutputFuncsTable, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

/* Base Input Pin */
HRESULT WINAPI BaseInputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI BaseInputPinImpl_Release(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_EndOfStream(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_BeginFlush(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_EndFlush(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

HRESULT BaseInputPin_Construct(const IPinVtbl *InputPin_Vtbl, const PIN_INFO * pPinInfo, const BasePinFuncTable* pBaseFuncsTable, const BaseInputPinFuncTable* pBaseInputFuncsTable, LPCRITICAL_SECTION pCritSec, IMemAllocator *, IPin ** ppPin);

typedef struct BaseFilter
{
	const struct IBaseFilterVtbl *lpVtbl;
	LONG refCount;
	CRITICAL_SECTION csFilter;

	FILTER_STATE state;
	REFERENCE_TIME rtStreamStart;
	IReferenceClock * pClock;
	FILTER_INFO filterInfo;
	CLSID clsid;
	LONG pinVersion;

	const struct BaseFilterFuncTable* pFuncsTable;
} BaseFilter;

typedef IPin* (WINAPI *BaseFilter_GetPin)(BaseFilter* iface, int iPosition);
typedef LONG (WINAPI *BaseFilter_GetPinCount)(BaseFilter* iface);
typedef LONG (WINAPI *BaseFilter_GetPinVersion)(BaseFilter* iface);

typedef struct BaseFilterFuncTable {
	/* Required */
	BaseFilter_GetPin pfnGetPin;
	BaseFilter_GetPinCount pfnGetPinCount;
} BaseFilterFuncTable;

HRESULT WINAPI BaseFilterImpl_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI BaseFilterImpl_AddRef(IBaseFilter * iface);
ULONG WINAPI BaseFilterImpl_Release(IBaseFilter * iface);
HRESULT WINAPI BaseFilterImpl_GetClassID(IBaseFilter * iface, CLSID * pClsid);
HRESULT WINAPI BaseFilterImpl_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState );
HRESULT WINAPI BaseFilterImpl_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock);
HRESULT WINAPI BaseFilterImpl_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock);
HRESULT WINAPI BaseFilterImpl_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum);
HRESULT WINAPI BaseFilterImpl_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo);
HRESULT WINAPI BaseFilterImpl_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName );
HRESULT WINAPI BaseFilterImpl_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo);

LONG WINAPI BaseFilterImpl_GetPinVersion(BaseFilter* This);
VOID WINAPI BaseFilterImpl_IncrementPinVersion(BaseFilter* This);

HRESULT WINAPI BaseFilter_Init(BaseFilter * This, const IBaseFilterVtbl *Vtbl, const CLSID *pClsid, DWORD_PTR DebugInfo, const BaseFilterFuncTable* pBaseFuncsTable);

/* Enums */
HRESULT WINAPI EnumMediaTypes_Construct(BasePin *iface, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum);

HRESULT WINAPI EnumPins_Construct(BaseFilter *base,  BaseFilter_GetPin receive_pin, BaseFilter_GetPinCount receive_pincount, BaseFilter_GetPinVersion receive_version, IEnumPins ** ppEnum);

/* Transform Filter */
typedef struct TransformFilter
{
	BaseFilter filter;

	IPin **ppPins;
	ULONG npins;
	AM_MEDIA_TYPE pmt;

	const struct TransformFilterFuncTable * pFuncsTable;
} TransformFilter;

typedef HRESULT (WINAPI *TransformFilter_DecideBufferSize) (TransformFilter *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
typedef HRESULT (WINAPI *TransformFilter_StartStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_StopStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_Receive) (TransformFilter* iface, IMediaSample* pIn);
typedef HRESULT (WINAPI *TransformFilter_CompleteConnect) (TransformFilter *iface, PIN_DIRECTION dir, IPin *pPin);
typedef HRESULT (WINAPI *TransformFilter_BreakConnect) (TransformFilter *iface, PIN_DIRECTION dir);
typedef HRESULT (WINAPI *TransformFilter_SetMediaType) (TransformFilter *iface, PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType);
typedef HRESULT (WINAPI *TransformFilter_CheckInputType) (TransformFilter *iface, const AM_MEDIA_TYPE *pMediaType);
typedef HRESULT (WINAPI *TransformFilter_EndOfStream) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_BeginFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_EndFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_NewSegment) (TransformFilter *iface,
REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

typedef struct TransformFilterFuncTable {
	/* Required */
	TransformFilter_DecideBufferSize pfnDecideBufferSize;
	/* Optional */
	TransformFilter_StartStreaming pfnStartStreaming;
	TransformFilter_Receive pfnReceive;
	TransformFilter_StopStreaming pfnStopStreaming;
	TransformFilter_CheckInputType pfnCheckInputType;
	TransformFilter_SetMediaType pfnSetMediaType;
	TransformFilter_CompleteConnect pfnCompleteConnect;
	TransformFilter_BreakConnect pfnBreakConnect;
	TransformFilter_EndOfStream pfnEndOfStream;
	TransformFilter_BeginFlush pfnBeginFlush;
	TransformFilter_EndFlush pfnEndFlush;
	TransformFilter_NewSegment pfnNewSegment;
} TransformFilterFuncTable;

HRESULT WINAPI TransformFilterImpl_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI TransformFilterImpl_Release(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Stop(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Pause(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Run(IBaseFilter * iface, REFERENCE_TIME tStart);
HRESULT WINAPI TransformFilterImpl_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin);

HRESULT TransformFilter_Construct( const IBaseFilterVtbl *filterVtbl, LONG filter_size, const CLSID* pClsid, const TransformFilterFuncTable* pFuncsTable, IBaseFilter ** ppTransformFilter);

/* Source Seeking */
typedef HRESULT (WINAPI *SourceSeeking_ChangeRate)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStart)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStop)(IMediaSeeking *iface);

typedef struct SourceSeeking
{
	const IMediaSeekingVtbl * lpVtbl;

	ULONG refCount;
	SourceSeeking_ChangeStop fnChangeStop;
	SourceSeeking_ChangeStart fnChangeStart;
	SourceSeeking_ChangeRate fnChangeRate;
	DWORD dwCapabilities;
	double dRate;
	LONGLONG llCurrent, llStop, llDuration;
	GUID timeformat;
	PCRITICAL_SECTION crst;
} SourceSeeking;

HRESULT SourceSeeking_Init(SourceSeeking *pSeeking, const IMediaSeekingVtbl *Vtbl, SourceSeeking_ChangeStop fnChangeStop, SourceSeeking_ChangeStart fnChangeStart, SourceSeeking_ChangeRate fnChangeRate, PCRITICAL_SECTION crit_sect);

HRESULT WINAPI SourceSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration);
HRESULT WINAPI SourceSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent);
HRESULT WINAPI SourceSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat);
HRESULT WINAPI SourceSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags);
HRESULT WINAPI SourceSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest);
HRESULT WINAPI SourceSeekingImpl_SetRate(IMediaSeeking * iface, double dRate);
HRESULT WINAPI SourceSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate);
HRESULT WINAPI SourceSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll);

/* Filter Registration */

typedef REGPINTYPES AMOVIESETUP_MEDIATYPE;
typedef REGFILTERPINS AMOVIESETUP_PIN;

typedef struct AMOVIESETUP_FILTER {
	const CLSID *clsid;
	const WCHAR *name;
	DWORD merit;
	UINT pins;
	const AMOVIESETUP_PIN *pPin;
} AMOVIESETUP_FILTER, *LPAMOVIESETUP_FILTER;

typedef IUnknown *(CALLBACK *LPFNNewCOMObject)(LPUNKNOWN pUnkOuter, HRESULT *phr);
typedef void (CALLBACK *LPFNInitRoutine)(BOOL bLoading, const CLSID *rclsid);

typedef struct tagFactoryTemplate {
	const WCHAR *m_Name;
	const CLSID *m_ClsID;
	LPFNNewCOMObject m_lpfnNew;
	LPFNInitRoutine m_lpfnInit;
	const AMOVIESETUP_FILTER *m_pAMovieSetup_Filter;
} FactoryTemplate;

HRESULT WINAPI AMovieDllRegisterServer2(BOOL bRegister);
HRESULT WINAPI AMovieSetupRegisterFilter2(const AMOVIESETUP_FILTER *pFilter, IFilterMapper2  *pIFM2, BOOL  bRegister);

/* Dll Functions */
BOOL WINAPI STRMBASE_DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv);
HRESULT WINAPI STRMBASE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
HRESULT WINAPI STRMBASE_DllCanUnloadNow(void);
