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

#include "wine/list.h"

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);
void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType);

void strmbase_dump_media_type(const AM_MEDIA_TYPE *mt);

/* Pin functions */

struct strmbase_pin
{
    IPin IPin_iface;
    struct strmbase_filter *filter;
    PIN_DIRECTION dir;
    WCHAR name[128];
    IPin *peer;
    AM_MEDIA_TYPE mt;

    const struct strmbase_pin_ops *ops;
};

struct strmbase_pin_ops
{
    /* Required for QueryAccept(), Connect(), ReceiveConnection(). */
    HRESULT (*pin_query_accept)(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt);
    /* Required for EnumMediaTypes(). */
    HRESULT (*pin_get_media_type)(struct strmbase_pin *pin, unsigned int index, AM_MEDIA_TYPE *mt);
    HRESULT (*pin_query_interface)(struct strmbase_pin *pin, REFIID iid, void **out);
};

struct strmbase_source
{
    struct strmbase_pin pin;
    IMemInputPin *pMemInputPin;
    IMemAllocator *pAllocator;

    const struct strmbase_source_ops *pFuncsTable;
};

typedef HRESULT (WINAPI *BaseOutputPin_AttemptConnection)(struct strmbase_source *pin, IPin *peer, const AM_MEDIA_TYPE *mt);
typedef HRESULT (WINAPI *BaseOutputPin_DecideBufferSize)(struct strmbase_source *pin, IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props);
typedef HRESULT (WINAPI *BaseOutputPin_DecideAllocator)(struct strmbase_source *pin, IMemInputPin *peer, IMemAllocator **allocator);

struct strmbase_source_ops
{
    struct strmbase_pin_ops base;

    /* Required for Connect(). */
    BaseOutputPin_AttemptConnection pfnAttemptConnection;
    /* Required for BaseOutputPinImpl_DecideAllocator */
    BaseOutputPin_DecideBufferSize pfnDecideBufferSize;
    /* Required for BaseOutputPinImpl_AttemptConnection */
    BaseOutputPin_DecideAllocator pfnDecideAllocator;
};

struct strmbase_sink
{
    struct strmbase_pin pin;

    IMemInputPin IMemInputPin_iface;
    IMemAllocator *pAllocator;
    BOOL flushing;
    IMemAllocator *preferred_allocator;

    const struct strmbase_sink_ops *pFuncsTable;
};

typedef HRESULT (WINAPI *BaseInputPin_Receive)(struct strmbase_sink *This, IMediaSample *pSample);

struct strmbase_sink_ops
{
    struct strmbase_pin_ops base;
    BaseInputPin_Receive pfnReceive;
    HRESULT (*sink_connect)(struct strmbase_sink *pin, IPin *peer, const AM_MEDIA_TYPE *mt);
    void (*sink_disconnect)(struct strmbase_sink *pin);
    HRESULT (*sink_eos)(struct strmbase_sink *pin);
    HRESULT (*sink_begin_flush)(struct strmbase_sink *pin);
    HRESULT (*sink_end_flush)(struct strmbase_sink *pin);
    HRESULT (*sink_new_segment)(struct strmbase_sink *pin, REFERENCE_TIME start, REFERENCE_TIME stop, double rate);
};

/* Base Pin */
HRESULT strmbase_pin_get_media_type(struct strmbase_pin *pin, unsigned int index, AM_MEDIA_TYPE *mt);

HRESULT WINAPI BaseOutputPinImpl_GetDeliveryBuffer(struct strmbase_source *pin,
        IMediaSample **sample, REFERENCE_TIME *start, REFERENCE_TIME *stop, DWORD flags);
HRESULT WINAPI BaseOutputPinImpl_Active(struct strmbase_source *pin);
HRESULT WINAPI BaseOutputPinImpl_Inactive(struct strmbase_source *pin);
HRESULT WINAPI BaseOutputPinImpl_InitAllocator(struct strmbase_source *pin, IMemAllocator **allocator);
HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(struct strmbase_source *pin, IMemInputPin *peer, IMemAllocator **allocator);
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(struct strmbase_source *pin, IPin *peer, const AM_MEDIA_TYPE *mt);

void strmbase_source_cleanup(struct strmbase_source *pin);
void strmbase_source_init(struct strmbase_source *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_source_ops *func_table);

void strmbase_sink_init(struct strmbase_sink *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_sink_ops *ops, IMemAllocator *allocator);
void strmbase_sink_cleanup(struct strmbase_sink *pin);

struct strmbase_filter
{
    IBaseFilter IBaseFilter_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
    CRITICAL_SECTION csFilter;

    FILTER_STATE state;
    IReferenceClock *clock;
    WCHAR name[128];
    IFilterGraph *graph;
    CLSID clsid;
    LONG pin_version;

    const struct strmbase_filter_ops *ops;
};

struct strmbase_filter_ops
{
    struct strmbase_pin *(*filter_get_pin)(struct strmbase_filter *iface, unsigned int index);
    void (*filter_destroy)(struct strmbase_filter *iface);
    HRESULT (*filter_query_interface)(struct strmbase_filter *iface, REFIID iid, void **out);

    HRESULT (*filter_init_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_start_stream)(struct strmbase_filter *iface, REFERENCE_TIME time);
    HRESULT (*filter_stop_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_cleanup_stream)(struct strmbase_filter *iface);
    HRESULT (*filter_wait_state)(struct strmbase_filter *iface, DWORD timeout);
};

VOID WINAPI BaseFilterImpl_IncrementPinVersion(struct strmbase_filter *filter);

void strmbase_filter_init(struct strmbase_filter *filter, IUnknown *outer,
        const CLSID *clsid, const struct strmbase_filter_ops *func_table);
void strmbase_filter_cleanup(struct strmbase_filter *filter);

/* Transform Filter */
typedef struct TransformFilter
{
    struct strmbase_filter filter;

    struct strmbase_source source;
    IQualityControl source_IQualityControl_iface;
    IQualityControl *source_qc_sink;

    struct strmbase_sink sink;

    AM_MEDIA_TYPE pmt;
    CRITICAL_SECTION csReceive;

    const struct TransformFilterFuncTable * pFuncsTable;
    /* IMediaSeeking and IMediaPosition are implemented by ISeekingPassThru */
    IUnknown *seekthru_unk;
} TransformFilter;

typedef HRESULT (WINAPI *TransformFilter_DecideBufferSize) (TransformFilter *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
typedef HRESULT (WINAPI *TransformFilter_StartStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_StopStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_Receive) (TransformFilter* iface, IMediaSample* pIn);
typedef HRESULT (WINAPI *TransformFilter_BreakConnect) (TransformFilter *iface, PIN_DIRECTION dir);
typedef HRESULT (WINAPI *TransformFilter_CheckInputType) (TransformFilter *iface, const AM_MEDIA_TYPE *pMediaType);
typedef HRESULT (WINAPI *TransformFilter_EndOfStream) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_BeginFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_EndFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_NewSegment) (TransformFilter *iface,
REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
typedef HRESULT (WINAPI *TransformFilter_Notify) (TransformFilter *iface, IBaseFilter *sender, Quality qm);

typedef struct TransformFilterFuncTable {
	/* Required */
	TransformFilter_DecideBufferSize pfnDecideBufferSize;
	/* Optional */
	TransformFilter_StartStreaming pfnStartStreaming;
	TransformFilter_Receive pfnReceive;
	TransformFilter_StopStreaming pfnStopStreaming;
        HRESULT (*transform_connect_sink)(TransformFilter *filter, const AM_MEDIA_TYPE *mt);
	TransformFilter_BreakConnect pfnBreakConnect;
	TransformFilter_EndFlush pfnEndFlush;
	TransformFilter_Notify pfnNotify;
} TransformFilterFuncTable;

HRESULT WINAPI TransformFilterImpl_Notify(TransformFilter *iface, IBaseFilter *sender, Quality qm);

HRESULT strmbase_transform_create(LONG filter_size, IUnknown *outer, const CLSID *clsid,
        const TransformFilterFuncTable *func_table, IBaseFilter **filter);

/* Source Seeking */
typedef HRESULT (WINAPI *SourceSeeking_ChangeRate)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStart)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStop)(IMediaSeeking *iface);

typedef struct SourceSeeking
{
	IMediaSeeking IMediaSeeking_iface;

	ULONG refCount;
	SourceSeeking_ChangeStop fnChangeStop;
	SourceSeeking_ChangeStart fnChangeStart;
	SourceSeeking_ChangeRate fnChangeRate;
	DWORD dwCapabilities;
	double dRate;
	LONGLONG llCurrent, llStop, llDuration;
	GUID timeformat;
	CRITICAL_SECTION cs;
} SourceSeeking;

HRESULT strmbase_seeking_init(SourceSeeking *seeking, const IMediaSeekingVtbl *vtbl,
        SourceSeeking_ChangeStop fnChangeStop, SourceSeeking_ChangeStart fnChangeStart,
        SourceSeeking_ChangeRate fnChangeRate);
void strmbase_seeking_cleanup(SourceSeeking *seeking);

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

HRESULT WINAPI CreatePosPassThru(IUnknown* pUnkOuter, BOOL bRenderer, IPin *pPin, IUnknown **ppPassThru);
HRESULT WINAPI PosPassThru_Construct(IUnknown* pUnkOuter, LPVOID *ppPassThru);

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

/* Output Queue */
typedef struct tagOutputQueue {
    CRITICAL_SECTION csQueue;

    struct strmbase_source *pInputPin;

    HANDLE hThread;
    HANDLE hProcessQueue;

    LONG lBatchSize;
    BOOL bBatchExact;
    BOOL bTerminate;
    BOOL bSendAnyway;

    struct list SampleList;

    const struct OutputQueueFuncTable* pFuncsTable;
} OutputQueue;

typedef DWORD (WINAPI *OutputQueue_ThreadProc)(OutputQueue *This);

typedef struct OutputQueueFuncTable
{
    OutputQueue_ThreadProc pfnThreadProc;
} OutputQueueFuncTable;

HRESULT WINAPI OutputQueue_Construct( struct strmbase_source *pin, BOOL bAuto,
    BOOL bQueue, LONG lBatchSize, BOOL bBatchExact, DWORD dwPriority,
    const OutputQueueFuncTable* pFuncsTable, OutputQueue **ppOutputQueue );
HRESULT WINAPI OutputQueue_Destroy(OutputQueue *pOutputQueue);
HRESULT WINAPI OutputQueue_ReceiveMultiple(OutputQueue *pOutputQueue, IMediaSample **ppSamples, LONG nSamples, LONG *nSamplesProcessed);
HRESULT WINAPI OutputQueue_Receive(OutputQueue *pOutputQueue, IMediaSample *pSample);
VOID WINAPI OutputQueue_EOS(OutputQueue *pOutputQueue);
VOID WINAPI OutputQueue_SendAnyway(OutputQueue *pOutputQueue);
DWORD WINAPI OutputQueueImpl_ThreadProc(OutputQueue *pOutputQueue);

typedef struct tagBaseWindow
{
	HWND hWnd;
	LONG Width;
	LONG Height;

	const struct BaseWindowFuncTable* pFuncsTable;
} BaseWindow;

typedef RECT (WINAPI *BaseWindow_GetDefaultRect)(BaseWindow *This);
typedef BOOL (WINAPI *BaseWindow_OnSize)(BaseWindow *This, LONG Height, LONG Width);

typedef struct BaseWindowFuncTable
{
	/* Required */
	BaseWindow_GetDefaultRect pfnGetDefaultRect;
	/* Optional, WinProc Related */
	BaseWindow_OnSize pfnOnSize;
} BaseWindowFuncTable;

HRESULT WINAPI BaseWindow_Init(BaseWindow *pBaseWindow, const BaseWindowFuncTable* pFuncsTable);
HRESULT WINAPI BaseWindow_Destroy(BaseWindow *pBaseWindow);

HRESULT WINAPI BaseWindowImpl_PrepareWindow(BaseWindow *This);
HRESULT WINAPI BaseWindowImpl_DoneWithWindow(BaseWindow *This);

enum strmbase_type_id
{
    IBasicAudio_tid,
    IBasicVideo_tid,
    IMediaControl_tid,
    IMediaEvent_tid,
    IMediaPosition_tid,
    IVideoWindow_tid,
    last_tid
};

HRESULT strmbase_get_typeinfo(enum strmbase_type_id tid, ITypeInfo **typeinfo);

#ifdef __IVideoWindow_FWD_DEFINED__
typedef struct tagBaseControlWindow
{
	BaseWindow baseWindow;
	IVideoWindow IVideoWindow_iface;

	BOOL AutoShow;
	HWND hwndDrain;
	HWND hwndOwner;
	struct strmbase_filter *pFilter;
	struct strmbase_pin *pPin;
} BaseControlWindow;

HRESULT WINAPI strmbase_window_init(BaseControlWindow *window, const IVideoWindowVtbl *vtbl,
        struct strmbase_filter *filter, struct strmbase_pin *pin, const BaseWindowFuncTable *func_table);
HRESULT WINAPI BaseControlWindow_Destroy(BaseControlWindow *pControlWindow);

BOOL WINAPI BaseControlWindowImpl_PossiblyEatMessage(BaseWindow *This, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT WINAPI BaseControlWindowImpl_QueryInterface(IVideoWindow *iface, REFIID iid, void **out);
ULONG WINAPI BaseControlWindowImpl_AddRef(IVideoWindow *iface);
ULONG WINAPI BaseControlWindowImpl_Release(IVideoWindow *iface);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfoCount(IVideoWindow *iface, UINT*pctinfo);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfo(IVideoWindow *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo);
HRESULT WINAPI BaseControlWindowImpl_GetIDsOfNames(IVideoWindow *iface, REFIID riid, LPOLESTR*rgszNames, UINT cNames, LCID lcid, DISPID*rgDispId);
HRESULT WINAPI BaseControlWindowImpl_Invoke(IVideoWindow *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS*pDispParams, VARIANT*pVarResult, EXCEPINFO*pExepInfo, UINT*puArgErr);
HRESULT WINAPI BaseControlWindowImpl_put_Caption(IVideoWindow *iface, BSTR strCaption);
HRESULT WINAPI BaseControlWindowImpl_get_Caption(IVideoWindow *iface, BSTR *strCaption);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_put_AutoShow(IVideoWindow *iface, LONG AutoShow);
HRESULT WINAPI BaseControlWindowImpl_get_AutoShow(IVideoWindow *iface, LONG *AutoShow);
HRESULT WINAPI BaseControlWindowImpl_put_WindowState(IVideoWindow *iface, LONG WindowState);
HRESULT WINAPI BaseControlWindowImpl_get_WindowState(IVideoWindow *iface, LONG *WindowState);
HRESULT WINAPI BaseControlWindowImpl_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_get_BackgroundPalette(IVideoWindow *iface, LONG *pBackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_put_Visible(IVideoWindow *iface, LONG Visible);
HRESULT WINAPI BaseControlWindowImpl_get_Visible(IVideoWindow *iface, LONG *pVisible);
HRESULT WINAPI BaseControlWindowImpl_put_Left(IVideoWindow *iface, LONG Left);
HRESULT WINAPI BaseControlWindowImpl_get_Left(IVideoWindow *iface, LONG *pLeft);
HRESULT WINAPI BaseControlWindowImpl_put_Width(IVideoWindow *iface, LONG Width);
HRESULT WINAPI BaseControlWindowImpl_get_Width(IVideoWindow *iface, LONG *pWidth);
HRESULT WINAPI BaseControlWindowImpl_put_Top(IVideoWindow *iface, LONG Top);
HRESULT WINAPI BaseControlWindowImpl_get_Top(IVideoWindow *iface, LONG *pTop);

HRESULT WINAPI BaseControlWindowImpl_put_Height(IVideoWindow *iface, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_get_Height(IVideoWindow *iface, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_put_Owner(IVideoWindow *iface, OAHWND Owner);
HRESULT WINAPI BaseControlWindowImpl_get_Owner(IVideoWindow *iface, OAHWND *Owner);
HRESULT WINAPI BaseControlWindowImpl_put_MessageDrain(IVideoWindow *iface, OAHWND Drain);
HRESULT WINAPI BaseControlWindowImpl_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain);
HRESULT WINAPI BaseControlWindowImpl_get_BorderColor(IVideoWindow *iface, LONG *Color);
HRESULT WINAPI BaseControlWindowImpl_put_BorderColor(IVideoWindow *iface, LONG Color);
HRESULT WINAPI BaseControlWindowImpl_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_SetWindowForeground(IVideoWindow *iface, LONG Focus);
HRESULT WINAPI BaseControlWindowImpl_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top, LONG Width, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg, LONG_PTR wParam, LONG_PTR lParam);
HRESULT WINAPI BaseControlWindowImpl_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_HideCursor(IVideoWindow *iface, LONG HideCursor);
HRESULT WINAPI BaseControlWindowImpl_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden);
#endif

#ifdef __IBasicVideo_FWD_DEFINED__
#ifdef __amvideo_h__
typedef struct tagBaseControlVideo
{
	IBasicVideo IBasicVideo_iface;

	struct strmbase_filter *pFilter;
	struct strmbase_pin *pPin;

	const struct BaseControlVideoFuncTable* pFuncsTable;
} BaseControlVideo;

typedef HRESULT (WINAPI *BaseControlVideo_GetSourceRect)(BaseControlVideo* This, RECT *pSourceRect);
typedef HRESULT (WINAPI *BaseControlVideo_GetStaticImage)(BaseControlVideo* This, LONG *pBufferSize, LONG *pDIBImage);
typedef HRESULT (WINAPI *BaseControlVideo_GetTargetRect)(BaseControlVideo* This, RECT *pTargetRect);
typedef VIDEOINFOHEADER* (WINAPI *BaseControlVideo_GetVideoFormat)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_IsDefaultSourceRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_IsDefaultTargetRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetDefaultSourceRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetDefaultTargetRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetSourceRect)(BaseControlVideo* This, RECT *pSourceRect);
typedef HRESULT (WINAPI *BaseControlVideo_SetTargetRect)(BaseControlVideo* This, RECT *pTargetRect);

typedef struct BaseControlVideoFuncTable {
	/* Required */
	BaseControlVideo_GetSourceRect pfnGetSourceRect;
	BaseControlVideo_GetStaticImage pfnGetStaticImage;
	BaseControlVideo_GetTargetRect pfnGetTargetRect;
	BaseControlVideo_GetVideoFormat pfnGetVideoFormat;
	BaseControlVideo_IsDefaultSourceRect pfnIsDefaultSourceRect;
	BaseControlVideo_IsDefaultTargetRect pfnIsDefaultTargetRect;
	BaseControlVideo_SetDefaultSourceRect pfnSetDefaultSourceRect;
	BaseControlVideo_SetDefaultTargetRect pfnSetDefaultTargetRect;
	BaseControlVideo_SetSourceRect pfnSetSourceRect;
	BaseControlVideo_SetTargetRect pfnSetTargetRect;
} BaseControlVideoFuncTable;

HRESULT WINAPI strmbase_video_init(BaseControlVideo *video, struct strmbase_filter *filter,
        struct strmbase_pin *pin, const BaseControlVideoFuncTable *func_table);
HRESULT WINAPI BaseControlVideo_Destroy(BaseControlVideo *pControlVideo);
#endif
#endif

struct strmbase_renderer
{
    struct strmbase_filter filter;

    struct strmbase_sink sink;
    IUnknown *pPosition;
    CRITICAL_SECTION csRenderLock;
    /* Signaled when the filter has completed a state change. The filter waits
     * for this event in IBaseFilter::GetState(). */
    HANDLE state_event;
    /* Signaled when the sample presentation time occurs. The streaming thread
     * waits for this event in Receive() if applicable. */
    HANDLE advise_event;
    /* Signaled when a flush or state change occurs, i.e. anything that needs
     * to immediately unblock the streaming thread. */
    HANDLE flush_event;
    IMediaSample *pMediaSample;
    REFERENCE_TIME stream_start;

    IQualityControl *pQSink;
    struct QualityControlImpl *qcimpl;

    const struct strmbase_renderer_ops *pFuncsTable;

    BOOL eos;
};

typedef HRESULT (WINAPI *BaseRenderer_CheckMediaType)(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt);
typedef HRESULT (WINAPI *BaseRenderer_DoRenderSample)(struct strmbase_renderer *iface, IMediaSample *sample);
typedef HRESULT (WINAPI *BaseRenderer_ShouldDrawSampleNow)(struct strmbase_renderer *iface,
        IMediaSample *sample, REFERENCE_TIME *start, REFERENCE_TIME *end);
typedef HRESULT (WINAPI *BaseRenderer_PrepareReceive)(struct strmbase_renderer *iface, IMediaSample *sample);
typedef HRESULT (WINAPI *BaseRenderer_EndOfStream)(struct strmbase_renderer *iface);
typedef HRESULT (WINAPI *BaseRenderer_BeginFlush) (struct strmbase_renderer *iface);
typedef HRESULT (WINAPI *BaseRenderer_EndFlush) (struct strmbase_renderer *iface);
typedef HRESULT (WINAPI *BaseRenderer_BreakConnect) (struct strmbase_renderer *iface);

struct strmbase_renderer_ops
{
    BaseRenderer_CheckMediaType pfnCheckMediaType;
    BaseRenderer_DoRenderSample pfnDoRenderSample;
    void (*renderer_init_stream)(struct strmbase_renderer *iface);
    void (*renderer_start_stream)(struct strmbase_renderer *iface);
    void (*renderer_stop_stream)(struct strmbase_renderer *iface);
    BaseRenderer_ShouldDrawSampleNow  pfnShouldDrawSampleNow;
    BaseRenderer_PrepareReceive pfnPrepareReceive;
    HRESULT (*renderer_connect)(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt);
    BaseRenderer_BreakConnect pfnBreakConnect;
    BaseRenderer_EndOfStream pfnEndOfStream;
    BaseRenderer_EndFlush pfnEndFlush;
    void (*renderer_destroy)(struct strmbase_renderer *iface);
    HRESULT (*renderer_query_interface)(struct strmbase_renderer *iface, REFIID iid, void **out);
    HRESULT (*renderer_pin_query_interface)(struct strmbase_renderer *iface, REFIID iid, void **out);
};

HRESULT WINAPI BaseRendererImpl_ClearPendingSample(struct strmbase_renderer *filter);
HRESULT WINAPI BaseRendererImpl_Receive(struct strmbase_renderer *filter, IMediaSample *sample);

HRESULT WINAPI strmbase_renderer_init(struct strmbase_renderer *filter, IUnknown *outer,
        const CLSID *clsid, const WCHAR *sink_name, const struct strmbase_renderer_ops *ops);
void strmbase_renderer_cleanup(struct strmbase_renderer *filter);

/* Dll Functions */
BOOL WINAPI STRMBASE_DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv);
HRESULT WINAPI STRMBASE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
HRESULT WINAPI STRMBASE_DllCanUnloadNow(void);
