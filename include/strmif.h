#ifndef __WINE_STRMIF_H_
#define __WINE_STRMIF_H_

#include "ole2.h"

/* FIXME - far from complete. */

/* forward decls. */

typedef struct IAMAnalogVideoDecoder IAMAnalogVideoDecoder;
typedef struct IAMAnalogVideoEncoder IAMAnalogVideoEncoder;
typedef struct IAMAudioInputMixer IAMAudioInputMixer;
typedef struct IAMAudioRendererStats IAMAudioRendererStats;
typedef struct IAMBufferNegotiation IAMBufferNegotiation;
typedef struct IAMCameraControl IAMCameraControl;
typedef struct IAMClockAdjust IAMClockAdjust;
typedef struct IAMCopyCaptureFileProgress IAMCopyCaptureFileProgress;
typedef struct IAMCrossbar IAMCrossbar;
typedef struct IAMDeviceRemoval IAMDeviceRemoval;
typedef struct IAMDevMemoryAllocator IAMDevMemoryAllocator;
typedef struct IAMDevMemoryControl IAMDevMemoryControl;
typedef struct IAMDroppedFrames IAMDroppedFrames;
typedef struct IAMExtDevice IAMExtDevice;
typedef struct IAMExtTransport IAMExtTransport;
typedef struct IAMFilterMiscFlags IAMFilterMiscFlags;
typedef struct IAMGraphStreams IAMGraphStreams;
typedef struct IAMLatency IAMLatency;
typedef struct IAMOpenProgress IAMOpenProgress;
typedef struct IAMOverlayFX IAMOverlayFX;
typedef struct IAMovie IAMovie;
typedef struct IAMovieSetup IAMovieSetup;
typedef struct IAMPhysicalPinInfo IAMPhysicalPinInfo;
typedef struct IAMPushSource IAMPushSource;
typedef struct IAMResourceControl IAMResourceControl;
typedef struct IAMStreamConfig IAMStreamConfig;
typedef struct IAMStreamControl IAMStreamControl;
typedef struct IAMStreamSelect IAMStreamSelect;
typedef struct IAMTimecodeDisplay IAMTimecodeDisplay;
typedef struct IAMTimecodeGenerator IAMTimecodeGenerator;
typedef struct IAMTimecodeReader IAMTimecodeReader;
typedef struct IAMTuner IAMTuner;
typedef struct IAMTunerNotification IAMTunerNotification;
typedef struct IAMTVAudio IAMTVAudio;
typedef struct IAMTVAudioNotification IAMTVAudioNotification;
typedef struct IAMTVTuner IAMTVTuner;
typedef struct IAMVfwCaptureDialogs IAMVfwCaptureDialogs;
typedef struct IAMVfwCompressDialogs IAMVfwCompressDialogs;
typedef struct IAMVideoCompression IAMVideoCompression;
typedef struct IAMVideoControl IAMVideoControl;
typedef struct IAMVideoDecimationProperties IAMVideoDecimationProperties;
typedef struct IAMVideoProcAmp IAMVideoProcAmp;
typedef struct IAsyncReader IAsyncReader;
typedef struct IMediaFilter IMediaFilter;
typedef struct IBaseFilter IBaseFilter;
typedef struct IBPCSatelliteTuner IBPCSatelliteTuner;
typedef struct ICaptureGraphBuilder ICaptureGraphBuilder;
typedef struct ICaptureGraphBuilder2 ICaptureGraphBuilder2;
typedef struct IConfigAviMux IConfigAviMux;
typedef struct IConfigInterleaving IConfigInterleaving;
typedef struct ICreateDevEnum ICreateDevEnum;
typedef struct IDDrawExclModeVideo IDDrawExclModeVideo;
typedef struct IDDrawExclModeVideoCallback IDDrawExclModeVideoCallback;
typedef struct IDecimateVideoImage IDecimateVideoImage;
typedef struct IDistributorNotify IDistributorNotify;
typedef struct IDrawVideoImage IDrawVideoImage;
typedef struct IDvdCmd IDvdCmd;
typedef struct IDvdControl IDvdControl;
typedef struct IDvdControl2 IDvdControl2;
typedef struct IDvdGraphBuilder IDvdGraphBuilder;
typedef struct IDvdInfo IDvdInfo;
typedef struct IDvdInfo2 IDvdInfo2;
typedef struct IDvdState IDvdState;
typedef struct IDVEnc IDVEnc;
typedef struct IDVSplitter IDVSplitter;
typedef struct IEnumFilters IEnumFilters;
typedef struct IEnumMediaTypes IEnumMediaTypes;
typedef struct IEnumPins IEnumPins;
typedef struct IEnumRegFilters IEnumRegFilters;
typedef struct IEnumStreamIdMap IEnumStreamIdMap;
typedef struct IFileSinkFilter IFileSinkFilter;
typedef struct IFileSinkFilter2 IFileSinkFilter2;
typedef struct IFileSourceFilter IFileSourceFilter;
typedef struct IFilterChain IFilterChain;
typedef struct IFilterGraph IFilterGraph;
typedef struct IFilterGraph2 IFilterGraph2;
typedef struct IFilterMapper IFilterMapper;
typedef struct IFilterMapper2 IFilterMapper2;
typedef struct IFilterMapper3 IFilterMapper3;
typedef struct IGraphBuilder IGraphBuilder;
typedef struct IGraphConfig IGraphConfig;
typedef struct IGraphConfigCallback IGraphConfigCallback;
typedef struct IGraphVersion IGraphVersion;
typedef struct IIPDVDec IIPDVDec;
typedef struct IMediaEventSink IMediaEventSink;
typedef struct IMediaPropertyBag IMediaPropertyBag;
typedef struct IMediaSample IMediaSample;
typedef struct IMediaSample2 IMediaSample2;
typedef struct IMediaSeeking IMediaSeeking;
typedef struct IMemAllocator IMemAllocator;
typedef struct IMemAllocatorCallbackTemp IMemAllocatorCallbackTemp;
typedef struct IMemAllocatorNotifyCallbackTemp IMemAllocatorNotifyCallbackTemp;
typedef struct IMemInputPin IMemInputPin;
typedef struct IMpeg2Demultiplexer IMpeg2Demultiplexer;
typedef struct IMPEG2StreamIdMap IMPEG2StreamIdMap;
typedef struct IOverlay IOverlay;
typedef struct IOverlayNotify IOverlayNotify;
typedef struct IOverlayNotify2 IOverlayNotify2;
typedef struct IPersistMediaPropertyBag IPersistMediaPropertyBag;
typedef struct IPin IPin;
typedef struct IPinConnection IPinConnection;
typedef struct IPinFlowControl IPinFlowControl;
typedef struct IQualityControl IQualityControl;
typedef struct IReferenceClock IReferenceClock;
typedef struct IReferenceClock2 IReferenceClock2;
typedef struct IResourceConsumer IResourceConsumer;
typedef struct IResourceManager IResourceManager;
typedef struct ISeekingPassThru ISeekingPassThru;
typedef struct IStreamBuilder IStreamBuilder;
typedef struct IVideoFrameStep IVideoFrameStep;

#ifndef __WINE_REFTIME_DEFINED_
#define __WINE_REFTIME_DEFINED_
typedef double REFTIME;
#endif /* __WINE_REFTIME_DEFINED_ */

typedef LONGLONG	REFERENCE_TIME;
typedef DWORD_PTR	HSEMAPHORE;
typedef DWORD_PTR	HEVENT;

/* enums. */

typedef enum
{
	PINDIR_INPUT = 0,
	PINDIR_OUTPUT = 1,
} PIN_DIRECTION;

typedef enum
{
	State_Stopped = 0,
	State_Paused = 1,
	State_Running = 2,
} FILTER_STATE;

/* structs. */

typedef struct
{
	GUID	majortype;
	GUID	subtype;
	BOOL	bFixedSizeSamples;
	BOOL	bTemporalCompression;
	ULONG	lSampleSize;
	GUID	formattype;
	IUnknown*	pUnk;
	ULONG	cbFormat;
	BYTE*	pbFormat;
} AM_MEDIA_TYPE;

typedef struct
{
	long	cBuffers;
	long	cbBuffer;
	long	cbAlign;
	long	cbPrefix;
} ALLOCATOR_PROPERTIES;

typedef struct
{
	IBaseFilter*	pFilter;
	PIN_DIRECTION	dir;
	WCHAR		achName[ 128 ];
} PIN_INFO;

typedef struct
{
	WCHAR		achName[ 128 ];
	IFilterGraph*	pGraph;
} FILTER_INFO;

typedef struct
{
	REFERENCE_TIME	tStart;
	REFERENCE_TIME	tStop;
	DWORD		dwStartCookie;
	DWORD		dwStopCookie;
	DWORD		dwFlags;
} AM_STREAM_INFO;

typedef struct tagCOLORKEY
{
	DWORD		KeyType;
	DWORD		PaletteIndex;
	COLORREF	LowColorValue;
	COLORREF	HighColorValue;
} COLORKEY;



/* defines. */

#define CHARS_IN_GUID	39
#define MAX_PIN_NAME	128
#define MAX_FILTER_NAME	128


/* interfaces. */

/**************************************************************************
 *
 * IMediaFilter interface
 *
 */

#define ICOM_INTERFACE IMediaFilter
#define IMediaFilter_METHODS \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD (HRESULT,Pause) \
    ICOM_METHOD1(HRESULT,Run,REFERENCE_TIME,a1) \
    ICOM_METHOD2(HRESULT,GetState,DWORD,a1,FILTER_STATE*,a2) \
    ICOM_METHOD1(HRESULT,SetSyncSource,IReferenceClock*,a1) \
    ICOM_METHOD1(HRESULT,GetSyncSource,IReferenceClock**,a1)

#define IMediaFilter_IMETHODS \
    IPersist_IMETHODS \
    IMediaFilter_METHODS

ICOM_DEFINE(IMediaFilter,IPersist)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaFilter_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaFilter_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaFilter_Release(p) ICOM_CALL (Release,p)
    /*** IPersist methods ***/
#define IMediaFilter_GetClassID(p,a1) ICOM_CALL1(GetClassID,p,a1)
    /*** IMediaFilter methods ***/
#define IMediaFilter_Stop(p) ICOM_CALL (Stop,p)
#define IMediaFilter_Pause(p) ICOM_CALL (Pause,p)
#define IMediaFilter_Run(p,a1) ICOM_CALL1(Run,p,a1)
#define IMediaFilter_GetState(p,a1,a2) ICOM_CALL2(GetState,p,a1,a2)
#define IMediaFilter_SetSyncSource(p,a1) ICOM_CALL1(SetSyncSource,p,a1)
#define IMediaFilter_GetSyncSource(p,a1) ICOM_CALL1(GetSyncSource,p,a1)

/**************************************************************************
 *
 * IBaseFilter interface
 *
 */

#define ICOM_INTERFACE IBaseFilter
#define IBaseFilter_METHODS \
    ICOM_METHOD1(HRESULT,EnumPins,IEnumPins**,a1) \
    ICOM_METHOD2(HRESULT,FindPin,LPCWSTR,a1,IPin**,a2) \
    ICOM_METHOD1(HRESULT,QueryFilterInfo,FILTER_INFO*,a1) \
    ICOM_METHOD2(HRESULT,JoinFilterGraph,IFilterGraph*,a1,LPCWSTR,a2) \
    ICOM_METHOD1(HRESULT,QueryVendorInfo,LPWSTR*,a1)

#define IBaseFilter_IMETHODS \
    IMediaFilter_IMETHODS \
    IBaseFilter_METHODS

ICOM_DEFINE(IBaseFilter,IMediaFilter)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IBaseFilter_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IBaseFilter_AddRef(p) ICOM_CALL (AddRef,p)
#define IBaseFilter_Release(p) ICOM_CALL (Release,p)
    /*** IPersist methods ***/
#define IBaseFilter_GetClassID(p,a1) ICOM_CALL1(GetClassID,p,a1)
    /*** IMediaFilter methods ***/
#define IBaseFilter_Stop(p,a1) ICOM_CALL1(Stop,p,a1)
#define IBaseFilter_Pause(p,a1) ICOM_CALL1(Pause,p,a1)
#define IBaseFilter_Run(p,a1) ICOM_CALL1(Run,p,a1)
#define IBaseFilter_GetState(p,a1,a2) ICOM_CALL2(GetState,p,a1,a2)
#define IBaseFilter_SetSyncSource(p,a1) ICOM_CALL1(SetSyncSource,p,a1)
#define IBaseFilter_GetSyncSource(p,a1) ICOM_CALL1(GetSyncSource,p,a1)
    /*** IBaseFilter methods ***/
#define IBaseFilter_EnumPins(p,a1) ICOM_CALL1(EnumPins,p,a1)
#define IBaseFilter_FindPin(p,a1,a2) ICOM_CALL2(FindPin,p,a1,a2)
#define IBaseFilter_QueryFilterInfo(p,a1) ICOM_CALL1(QueryFilterInfo,p,a1)
#define IBaseFilter_JoinFilterGraph(p,a1,a2) ICOM_CALL2(JoinFilterGraph,p,a1,a2)
#define IBaseFilter_QueryVendorInfo(p,a1) ICOM_CALL1(QueryVendorInfo,p,a1)

/**************************************************************************
 *
 * IEnumFilters interface
 *
 */

#define ICOM_INTERFACE IEnumFilters
#define IEnumFilters_METHODS \
    ICOM_METHOD3(HRESULT,Next,ULONG,a1,IBaseFilter**,a2,ULONG*,a3) \
    ICOM_METHOD1(HRESULT,Skip,ULONG,a1) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone,IEnumFilters**,a1)

#define IEnumFilters_IMETHODS \
    IUnknown_IMETHODS \
    IEnumFilters_METHODS

ICOM_DEFINE(IEnumFilters,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IEnumFilters_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IEnumFilters_AddRef(p) ICOM_CALL (AddRef,p)
#define IEnumFilters_Release(p) ICOM_CALL (Release,p)
    /*** IEnumFilters methods ***/
#define IEnumFilters_Next(p,a1,a2,a3) ICOM_CALL3(Next,p,a1,a2,a3)
#define IEnumFilters_Skip(p,a1) ICOM_CALL1(Skip,p,a1)
#define IEnumFilters_Reset(p) ICOM_CALL (Reset,p)
#define IEnumFilters_Clone(p,a1) ICOM_CALL1(Clone,p,a1)

/**************************************************************************
 *
 * IEnumMediaTypes interface
 *
 */

#define ICOM_INTERFACE IEnumMediaTypes
#define IEnumMediaTypes_METHODS \
    ICOM_METHOD3(HRESULT,Next,ULONG,a1,AM_MEDIA_TYPE**,a2,ULONG*,a3) \
    ICOM_METHOD1(HRESULT,Skip,ULONG,a1) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone,IEnumMediaTypes**,a1)

#define IEnumMediaTypes_IMETHODS \
    IUnknown_IMETHODS \
    IEnumMediaTypes_METHODS

ICOM_DEFINE(IEnumMediaTypes,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IEnumMediaTypes_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IEnumMediaTypes_AddRef(p) ICOM_CALL (AddRef,p)
#define IEnumMediaTypes_Release(p) ICOM_CALL (Release,p)
    /*** IEnumMediaTypes methods ***/
#define IEnumMediaTypes_Next(p,a1,a2,a3) ICOM_CALL3(Next,p,a1,a2,a3)
#define IEnumMediaTypes_Skip(p,a1) ICOM_CALL1(Skip,p,a1)
#define IEnumMediaTypes_Reset(p) ICOM_CALL (Reset,p)
#define IEnumMediaTypes_Clone(p,a1) ICOM_CALL1(Clone,p,a1)

/**************************************************************************
 *
 * IEnumPins interface
 *
 */

#define ICOM_INTERFACE IEnumPins
#define IEnumPins_METHODS \
    ICOM_METHOD3(HRESULT,Next,ULONG,a1,IPin**,a2,ULONG*,a3) \
    ICOM_METHOD1(HRESULT,Skip,ULONG,a1) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone,IEnumPins**,a1)

#define IEnumPins_IMETHODS \
    IUnknown_IMETHODS \
    IEnumPins_METHODS

ICOM_DEFINE(IEnumPins,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IEnumPins_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IEnumPins_AddRef(p) ICOM_CALL (AddRef,p)
#define IEnumPins_Release(p) ICOM_CALL (Release,p)
    /*** IEnumPins methods ***/
#define IEnumPins_Next(p,a1,a2,a3) ICOM_CALL3(Next,p,a1,a2,a3)
#define IEnumPins_Skip(p,a1) ICOM_CALL1(Skip,p,a1)
#define IEnumPins_Reset(p) ICOM_CALL (Reset,p)
#define IEnumPins_Clone(p,a1) ICOM_CALL1(Clone,p,a1)

/**************************************************************************
 *
 * IFileSourceFilter interface
 *
 */

#define ICOM_INTERFACE IFileSourceFilter
#define IFileSourceFilter_METHODS \
    ICOM_METHOD2(HRESULT,Load,LPCOLESTR,a1,const AM_MEDIA_TYPE*,a2) \
    ICOM_METHOD2(HRESULT,GetCurFile,LPOLESTR*,a1,AM_MEDIA_TYPE*,a2)

#define IFileSourceFilter_IMETHODS \
    IUnknown_IMETHODS \
    IFileSourceFilter_METHODS

ICOM_DEFINE(IFileSourceFilter,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFileSourceFilter_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFileSourceFilter_AddRef(p) ICOM_CALL (AddRef,p)
#define IFileSourceFilter_Release(p) ICOM_CALL (Release,p)
    /*** IFileSourceFilter methods ***/
#define IFileSourceFilter_Load(p,a1,a2) ICOM_CALL2(Load,p,a1,a2)
#define IFileSourceFilter_GetCurFile(p,a1,a2) ICOM_CALL2(GetCurFile,p,a1,a2)

/**************************************************************************
 *
 * IFilterGraph interface
 *
 */

#define ICOM_INTERFACE IFilterGraph
#define IFilterGraph_METHODS \
    ICOM_METHOD2(HRESULT,AddFilter,IBaseFilter*,a1,LPCWSTR,a2) \
    ICOM_METHOD1(HRESULT,RemoveFilter,IBaseFilter*,a1) \
    ICOM_METHOD1(HRESULT,EnumFilters,IEnumFilters**,a1) \
    ICOM_METHOD2(HRESULT,FindFilterByName,LPCWSTR,a1,IBaseFilter**,a2) \
    ICOM_METHOD3(HRESULT,ConnectDirect,IPin*,a1,IPin*,a2,const AM_MEDIA_TYPE*,a3) \
    ICOM_METHOD1(HRESULT,Reconnect,IPin*,a1) \
    ICOM_METHOD1(HRESULT,Disconnect,IPin*,a1) \
    ICOM_METHOD (HRESULT,SetDefaultSyncSource)

#define IFilterGraph_IMETHODS \
    IUnknown_IMETHODS \
    IFilterGraph_METHODS

ICOM_DEFINE(IFilterGraph,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterGraph_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterGraph_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterGraph_Release(p) ICOM_CALL (Release,p)
    /*** IFilterGraph methods ***/
#define IFilterGraph_AddFilter(p,a1,a2) ICOM_CALL2(AddFilter,p,a1,a2)
#define IFilterGraph_RemoveFilter(p,a1) ICOM_CALL1(RemoveFilter,p,a1)
#define IFilterGraph_EnumFilters(p,a1) ICOM_CALL1(EnumFilters,p,a1)
#define IFilterGraph_FindFilterByName(p,a1,a2) ICOM_CALL2(FindFilterByName,p,a1,a2)
#define IFilterGraph_ConnectDirect(p,a1,a2,a3) ICOM_CALL3(ConnectDirect,p,a1,a2,a3)
#define IFilterGraph_Reconnect(p,a1) ICOM_CALL1(Reconnect,p,a1)
#define IFilterGraph_Disconnect(p,a1) ICOM_CALL1(Disconnect,p,a1)
#define IFilterGraph_SetDefaultSyncSource(p) ICOM_CALL (SetDefaultSyncSource,p)

/**************************************************************************
 *
 * IMediaSample interface
 *
 */

#define ICOM_INTERFACE IMediaSample
#define IMediaSample_METHODS \
    ICOM_METHOD1(HRESULT,GetPointer,BYTE**,a1) \
    ICOM_METHOD (long,GetSize) \
    ICOM_METHOD2(HRESULT,GetTime,REFERENCE_TIME*,a1,REFERENCE_TIME*,a2) \
    ICOM_METHOD2(HRESULT,SetTime,REFERENCE_TIME*,a1,REFERENCE_TIME*,a2) \
    ICOM_METHOD (HRESULT,IsSyncPoint) \
    ICOM_METHOD1(HRESULT,SetSyncPoint,BOOL,a1) \
    ICOM_METHOD (HRESULT,IsPreroll) \
    ICOM_METHOD1(HRESULT,SetPreroll,BOOL,a1) \
    ICOM_METHOD (long,GetActualDataLength) \
    ICOM_METHOD1(HRESULT,SetActualDataLength,long,a1) \
    ICOM_METHOD1(HRESULT,GetMediaType,AM_MEDIA_TYPE**,a1) \
    ICOM_METHOD1(HRESULT,SetMediaType,AM_MEDIA_TYPE*,a1) \
    ICOM_METHOD (HRESULT,IsDiscontinuity) \
    ICOM_METHOD1(HRESULT,SetDiscontinuity,BOOL,a1) \
    ICOM_METHOD2(HRESULT,GetMediaTime,LONGLONG*,a1,LONGLONG*,a2) \
    ICOM_METHOD2(HRESULT,SetMediaTime,LONGLONG*,a1,LONGLONG*,a2)

#define IMediaSample_IMETHODS \
    IUnknown_IMETHODS \
    IMediaSample_METHODS

ICOM_DEFINE(IMediaSample,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaSample_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaSample_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaSample_Release(p) ICOM_CALL (Release,p)
    /*** IMediaSample methods ***/
#define IMediaSample_GetPointer(p,a1) ICOM_CALL1(GetPointer,p,a1)
#define IMediaSample_GetSize(p) ICOM_CALL (GetSize,p)
#define IMediaSample_GetTime(p,a1,a2) ICOM_CALL2(GetTime,p,a1,a2)
#define IMediaSample_SetTime(p,a1,a2) ICOM_CALL2(SetTime,p,a1,a2)
#define IMediaSample_IsSyncPoint(p) ICOM_CALL (IsSyncPoint,p)
#define IMediaSample_SetSyncPoint(p,a1) ICOM_CALL1(SetSyncPoint,p,a1)
#define IMediaSample_IsPreroll(p) ICOM_CALL (IsPreroll,p)
#define IMediaSample_SetPreroll(p,a1) ICOM_CALL1(SetPreroll,p,a1)
#define IMediaSample_GetActualDataLength(p,a1) ICOM_CALL1(GetActualDataLength,p,a1)
#define IMediaSample_SetActualDataLength(p,a1) ICOM_CALL1(SetActualDataLength,p,a1)
#define IMediaSample_GetMediaType(p,a1) ICOM_CALL1(GetMediaType,p,a1)
#define IMediaSample_SetMediaType(p,a1) ICOM_CALL1(SetMediaType,p,a1)
#define IMediaSample_IsDiscontinuity(p) ICOM_CALL (IsDiscontinuity,p)
#define IMediaSample_SetDiscontinuity(p,a1) ICOM_CALL1(SetDiscontinuity,p,a1)
#define IMediaSample_GetMediaTime(p,a1,a2) ICOM_CALL2(GetMediaTime,p,a1,a2)
#define IMediaSample_SetMediaTime(p,a1,a2) ICOM_CALL2(SetMediaTime,p,a1,a2)

/**************************************************************************
 *
 * IMediaSample2 interface
 *
 */

#define ICOM_INTERFACE IMediaSample2
#define IMediaSample2_METHODS \
    ICOM_METHOD2(HRESULT,GetProperties,DWORD,a1,BYTE*,a2) \
    ICOM_METHOD2(HRESULT,SetProperties,DWORD,a1,const BYTE*,a2)

#define IMediaSample2_IMETHODS \
    IMediaSample_IMETHODS \
    IMediaSample2_METHODS

ICOM_DEFINE(IMediaSample2,IMediaSample)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaSample2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaSample2_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaSample2_Release(p) ICOM_CALL (Release,p)
    /*** IMediaSample methods ***/
#define IMediaSample2_GetPointer(p,a1) ICOM_CALL1(GetPointer,p,a1)
#define IMediaSample2_GetSize(p,a1) ICOM_CALL1(GetSize,p,a1)
#define IMediaSample2_GetTime(p,a1,a2) ICOM_CALL2(GetTime,p,a1,a2)
#define IMediaSample2_SetTime(p,a1,a2) ICOM_CALL2(SetTime,p,a1,a2)
#define IMediaSample2_IsSyncPoint(p,a1) ICOM_CALL1(IsSyncPoint,p,a1)
#define IMediaSample2_SetSyncPoint(p,a1) ICOM_CALL1(SetSyncPoint,p,a1)
#define IMediaSample2_IsPreroll(p,a1) ICOM_CALL1(IsPreroll,p,a1)
#define IMediaSample2_SetPreroll(p,a1) ICOM_CALL1(SetPreroll,p,a1)
#define IMediaSample2_GetActualDataLength(p,a1) ICOM_CALL1(GetActualDataLength,p,a1)
#define IMediaSample2_SetActualDataLength(p,a1) ICOM_CALL1(SetActualDataLength,p,a1)
#define IMediaSample2_GetMediaType(p,a1) ICOM_CALL1(GetMediaType,p,a1)
#define IMediaSample2_SetMediaType(p,a1) ICOM_CALL1(SetMediaType,p,a1)
#define IMediaSample2_IsDiscontinuity(p,a1) ICOM_CALL1(IsDiscontinuity,p,a1)
#define IMediaSample2_SetDiscontinuity(p,a1) ICOM_CALL1(SetDiscontinuity,p,a1)
#define IMediaSample2_GetMediaTime(p,a1,a2) ICOM_CALL2(GetMediaTime,p,a1,a2)
#define IMediaSample2_SetMediaTime(p,a1,a2) ICOM_CALL2(SetMediaTime,p,a1,a2)
    /*** IMediaSample2 methods ***/
#define IMediaSample2_GetProperties(p,a1,a2) ICOM_CALL2(GetProperties,p,a1,a2)
#define IMediaSample2_SetProperties(p,a1,a2) ICOM_CALL2(SetProperties,p,a1,a2)

/**************************************************************************
 *
 * IMemAllocator interface
 *
 */

#define ICOM_INTERFACE IMemAllocator
#define IMemAllocator_METHODS \
    ICOM_METHOD2(HRESULT,SetProperties,ALLOCATOR_PROPERTIES*,a1,ALLOCATOR_PROPERTIES*,a2) \
    ICOM_METHOD1(HRESULT,GetProperties,ALLOCATOR_PROPERTIES*,a1) \
    ICOM_METHOD (HRESULT,Commit) \
    ICOM_METHOD (HRESULT,Decommit) \
    ICOM_METHOD4(HRESULT,GetBuffer,IMediaSample**,a1,REFERENCE_TIME*,a2,REFERENCE_TIME*,a3,DWORD,a4) \
    ICOM_METHOD1(HRESULT,ReleaseBuffer,IMediaSample*,a1)

#define IMemAllocator_IMETHODS \
    IUnknown_IMETHODS \
    IMemAllocator_METHODS

ICOM_DEFINE(IMemAllocator,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMemAllocator_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMemAllocator_AddRef(p) ICOM_CALL (AddRef,p)
#define IMemAllocator_Release(p) ICOM_CALL (Release,p)
    /*** IMemAllocator methods ***/
#define IMemAllocator_SetProperties(p,a1,a2) ICOM_CALL2(SetProperties,p,a1,a2)
#define IMemAllocator_GetProperties(p,a1) ICOM_CALL1(GetProperties,p,a1)
#define IMemAllocator_Commit(p) ICOM_CALL (Commit,p)
#define IMemAllocator_Decommit(p) ICOM_CALL (Decommit,p)
#define IMemAllocator_GetBuffer(p,a1,a2,a3,a4) ICOM_CALL4(GetBuffer,p,a1,a2,a3,a4)
#define IMemAllocator_ReleaseBuffer(p,a1) ICOM_CALL1(ReleaseBuffer,p,a1)

/**************************************************************************
 *
 * IMemAllocatorCallbackTemp interface
 *
 */

#define ICOM_INTERFACE IMemAllocatorCallbackTemp
#define IMemAllocatorCallbackTemp_METHODS \
    ICOM_METHOD1(HRESULT,SetNotify,IMemAllocatorNotifyCallbackTemp*,a1) \
    ICOM_METHOD1(HRESULT,GetFreeCount,LONG*,a1)

#define IMemAllocatorCallbackTemp_IMETHODS \
    IMemAllocator_IMETHODS \
    IMemAllocatorCallbackTemp_METHODS

ICOM_DEFINE(IMemAllocatorCallbackTemp,IMemAllocator)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMemAllocatorCallbackTemp_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMemAllocatorCallbackTemp_AddRef(p) ICOM_CALL (AddRef,p)
#define IMemAllocatorCallbackTemp_Release(p) ICOM_CALL (Release,p)
    /*** IMemAllocator methods ***/
#define IMemAllocatorCallbackTemp_SetProperties(p,a1,a2) ICOM_CALL2(SetProperties,p,a1,a2)
#define IMemAllocatorCallbackTemp_GetProperties(p,a1) ICOM_CALL1(GetProperties,p,a1)
#define IMemAllocatorCallbackTemp_Commit(p,a1) ICOM_CALL1(Commit,p,a1)
#define IMemAllocatorCallbackTemp_Decommit(p,a1) ICOM_CALL1(Decommit,p,a1)
#define IMemAllocatorCallbackTemp_GetBuffer(p,a1,a2,a3,a4) ICOM_CALL4(GetBuffer,p,a1,a2,a3,a4)
#define IMemAllocatorCallbackTemp_ReleaseBuffer(p,a1) ICOM_CALL1(ReleaseBuffer,p,a1)
    /*** IMemAllocatorCallbackTemp methods ***/
#define IMemAllocatorCallbackTemp_SetNotify(p,a1) ICOM_CALL1(SetNotify,p,a1)
#define IMemAllocatorCallbackTemp_GetFreeCount(p,a1) ICOM_CALL1(GetFreeCount,p,a1)

/**************************************************************************
 *
 * IMemAllocatorNotifyCallbackTemp interface
 *
 */

#define ICOM_INTERFACE IMemAllocatorNotifyCallbackTemp
#define IMemAllocatorNotifyCallbackTemp_METHODS \
    ICOM_METHOD (HRESULT,NotifyRelease)

#define IMemAllocatorNotifyCallbackTemp_IMETHODS \
    IUnknown_IMETHODS \
    IMemAllocatorNotifyCallbackTemp_METHODS

ICOM_DEFINE(IMemAllocatorNotifyCallbackTemp,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMemAllocatorNotifyCallbackTemp_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMemAllocatorNotifyCallbackTemp_AddRef(p) ICOM_CALL (AddRef,p)
#define IMemAllocatorNotifyCallbackTemp_Release(p) ICOM_CALL (Release,p)
    /*** IMemAllocatorNotifyCallbackTemp methods ***/
#define IMemAllocatorNotifyCallbackTemp_NotifyRelease(p) ICOM_CALL (NotifyRelease,p)

/**************************************************************************
 *
 * IMemInputPin interface
 *
 */

#define ICOM_INTERFACE IMemInputPin
#define IMemInputPin_METHODS \
    ICOM_METHOD1(HRESULT,GetAllocator,IMemAllocator**,a1) \
    ICOM_METHOD2(HRESULT,NotifyAllocator,IMemAllocator*,a1,BOOL,a2) \
    ICOM_METHOD1(HRESULT,GetAllocatorRequirements,ALLOCATOR_PROPERTIES*,a1) \
    ICOM_METHOD1(HRESULT,Receive,IMediaSample*,a1) \
    ICOM_METHOD3(HRESULT,ReceiveMultiple,IMediaSample**,a1,long,a2,long*,a3) \
    ICOM_METHOD (HRESULT,ReceiveCanBlock)

#define IMemInputPin_IMETHODS \
    IUnknown_IMETHODS \
    IMemInputPin_METHODS

ICOM_DEFINE(IMemInputPin,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMemInputPin_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMemInputPin_AddRef(p) ICOM_CALL (AddRef,p)
#define IMemInputPin_Release(p) ICOM_CALL (Release,p)
    /*** IMemInputPin methods ***/
#define IMemInputPin_GetAllocator(p,a1) ICOM_CALL1(GetAllocator,p,a1)
#define IMemInputPin_NotifyAllocator(p,a1,a2) ICOM_CALL2(NotifyAllocator,p,a1,a2)
#define IMemInputPin_GetAllocatorRequirements(p,a1) ICOM_CALL1(GetAllocatorRequirements,p,a1)
#define IMemInputPin_Receive(p,a1) ICOM_CALL1(Receive,p,a1)
#define IMemInputPin_ReceiveMultiple(p,a1,a2,a3) ICOM_CALL3(ReceiveMultiple,p,a1,a2,a3)
#define IMemInputPin_ReceiveCanBlock(p) ICOM_CALL (ReceiveCanBlock,p)

/**************************************************************************
 *
 * IPin interface
 *
 */

#define ICOM_INTERFACE IPin
#define IPin_METHODS \
    ICOM_METHOD2(HRESULT,Connect,IPin*,a1,const AM_MEDIA_TYPE*,a2) \
    ICOM_METHOD2(HRESULT,ReceiveConnection,IPin*,a1,const AM_MEDIA_TYPE*,a2) \
    ICOM_METHOD (HRESULT,Disconnect) \
    ICOM_METHOD1(HRESULT,ConnectedTo,IPin**,a1) \
    ICOM_METHOD1(HRESULT,ConnectionMediaType,AM_MEDIA_TYPE*,a1) \
    ICOM_METHOD1(HRESULT,QueryPinInfo,PIN_INFO*,a1) \
    ICOM_METHOD1(HRESULT,QueryDirection,PIN_DIRECTION*,a1) \
    ICOM_METHOD1(HRESULT,QueryId,LPWSTR*,a1) \
    ICOM_METHOD1(HRESULT,QueryAccept,const AM_MEDIA_TYPE*,a1) \
    ICOM_METHOD1(HRESULT,EnumMediaTypes,IEnumMediaTypes**,a1) \
    ICOM_METHOD2(HRESULT,QueryInternalConnections,IPin**,a1,ULONG*,a2) \
    ICOM_METHOD (HRESULT,EndOfStream) \
    ICOM_METHOD (HRESULT,BeginFlush) \
    ICOM_METHOD (HRESULT,EndFlush) \
    ICOM_METHOD3(HRESULT,NewSegment,REFERENCE_TIME,a1,REFERENCE_TIME,a2,double,a3)

#define IPin_IMETHODS \
    IUnknown_IMETHODS \
    IPin_METHODS

ICOM_DEFINE(IPin,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IPin_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IPin_AddRef(p) ICOM_CALL (AddRef,p)
#define IPin_Release(p) ICOM_CALL (Release,p)
    /*** IPin methods ***/
#define IPin_Connect(p,a1,a2) ICOM_CALL2(Connect,p,a1,a2)
#define IPin_ReceiveConnection(p,a1,a2) ICOM_CALL2(ReceiveConnection,p,a1,a2)
#define IPin_Disconnect(p) ICOM_CALL (Disconnect,p)
#define IPin_ConnectedTo(p,a1) ICOM_CALL1(ConnectedTo,p,a1)
#define IPin_ConnectionMediaType(p,a1) ICOM_CALL1(ConnectionMediaType,p,a1)
#define IPin_QueryPinInfo(p,a1) ICOM_CALL1(QueryPinInfo,p,a1)
#define IPin_QueryDirection(p,a1) ICOM_CALL1(QueryDirection,p,a1)
#define IPin_QueryId(p,a1) ICOM_CALL1(QueryId,p,a1)
#define IPin_QueryAccept(p,a1) ICOM_CALL1(QueryAccept,p,a1)
#define IPin_EnumMediaTypes(p,a1) ICOM_CALL1(EnumMediaTypes,p,a1)
#define IPin_QueryInternalConnections(p,a1,a2) ICOM_CALL2(QueryInternalConnections,p,a1,a2)
#define IPin_EndOfStream(p) ICOM_CALL (EndOfStream,p)
#define IPin_BeginFlush(p) ICOM_CALL (BeginFlush,p)
#define IPin_EndFlush(p) ICOM_CALL (EndFlush,p)
#define IPin_NewSegment(p,a1,a2,a3) ICOM_CALL3(NewSegment,p,a1,a2,a3)

/**************************************************************************
 *
 * IReferenceClock interface
 *
 */

#define ICOM_INTERFACE IReferenceClock
#define IReferenceClock_METHODS \
    ICOM_METHOD1(HRESULT,GetTime,REFERENCE_TIME*,a1) \
    ICOM_METHOD4(HRESULT,AdviseTime,REFERENCE_TIME,a1,REFERENCE_TIME,a2,HEVENT,a3,DWORD_PTR*,a4) \
    ICOM_METHOD4(HRESULT,AdvisePeriodic,REFERENCE_TIME,a1,REFERENCE_TIME,a2,HSEMAPHORE,a3,DWORD_PTR*,a4) \
    ICOM_METHOD1(HRESULT,Unadvise,DWORD_PTR,a1)

#define IReferenceClock_IMETHODS \
    IUnknown_IMETHODS \
    IReferenceClock_METHODS

ICOM_DEFINE(IReferenceClock,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IReferenceClock_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IReferenceClock_AddRef(p) ICOM_CALL (AddRef,p)
#define IReferenceClock_Release(p) ICOM_CALL (Release,p)
    /*** IReferenceClock methods ***/
#define IReferenceClock_GetTime(p,a1) ICOM_CALL1(GetTime,p,a1)
#define IReferenceClock_AdviseTime(p,a1,a2,a3,a4) ICOM_CALL4(AdviseTime,p,a1,a2,a3,a4)
#define IReferenceClock_AdvisePeriodic(p,a1,a2,a3,a4) ICOM_CALL4(AdvisePeriodic,p,a1,a2,a3,a4)
#define IReferenceClock_Unadvise(p,a1) ICOM_CALL1(Unadvise,p,a1)

/**************************************************************************
 *
 * IReferenceClock2 interface
 *
 */

#define ICOM_INTERFACE IReferenceClock2
#define IReferenceClock2_METHODS

#define IReferenceClock2_IMETHODS \
    IReferenceClock_IMETHODS \
    IReferenceClock2_METHODS

ICOM_DEFINE(IReferenceClock2,IReferenceClock)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IReferenceClock2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IReferenceClock2_AddRef(p) ICOM_CALL (AddRef,p)
#define IReferenceClock2_Release(p) ICOM_CALL (Release,p)
    /*** IReferenceClock methods ***/
#define IReferenceClock2_GetTime(p,a1) ICOM_CALL1(GetTime,p,a1)
#define IReferenceClock2_AdviseTime(p,a1,a2,a3,a4) ICOM_CALL4(AdviseTime,p,a1,a2,a3,a4)
#define IReferenceClock2_AdvisePeriodic(p,a1,a2,a3,a4) ICOM_CALL4(AdvisePeriodic,p,a1,a2,a3,a4)
#define IReferenceClock2_Unadvise(p,a1) ICOM_CALL1(Unadvise,p,a1)
    /*** IReferenceClock2 methods ***/

#endif  /* __WINE_STRMIF_H_ */
