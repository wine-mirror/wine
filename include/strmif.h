#ifndef __WINE__
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif
#endif

#ifndef __WINE_STRMIF_H_
#define __WINE_STRMIF_H_

#include "ole2.h"
#include "wine/obj_oleaut.h"
#include "wine/obj_property.h"
#include "wine/obj_ksproperty.h"

/* undef GetTimeFormat - FIXME? */
#undef GetTimeFormat

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

/* GUIDs. */

DEFINE_GUID(IID_IAMAnalogVideoDecoder,0xC6E13350,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMAnalogVideoEncoder,0xC6E133B0,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMAudioInputMixer,0x54C39221,0x8380,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_IAMAudioRendererStats,0x22320CB2,0xD41A,0x11D2,0xBF,0x7C,0xD7,0xCB,0x9D,0xF0,0xBF,0x93);
DEFINE_GUID(IID_IAMBufferNegotiation,0x56ED71A0,0xAF5F,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_IAMCameraControl,0xC6E13370,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMClockAdjust,0x4D5466B0,0xA49C,0x11D1,0xAB,0xE8,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IAMCopyCaptureFileProgress,0x670D1D20,0xA068,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_IAMCrossbar,0xC6E13380,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMDeviceRemoval,0xF90A6130,0xB658,0x11D2,0xAE,0x49,0x00,0x00,0xF8,0x75,0x4B,0x99);
DEFINE_GUID(IID_IAMDevMemoryAllocator,0xC6545BF0,0xE76B,0x11D0,0xBD,0x52,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IAMDevMemoryControl,0xC6545BF1,0xE76B,0x11D0,0xBD,0x52,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IAMDroppedFrames,0xC6E13344,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMExtDevice,0xB5730A90,0x1A2C,0x11CF,0x8C,0x23,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMExtTransport,0xA03CD5F0,0x3045,0x11CF,0x8C,0x44,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMFilterMiscFlags,0x2DD74950,0xA890,0x11D1,0xAB,0xE8,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IAMGraphStreams,0x632105FA,0x072E,0x11D3,0x8A,0xF9,0x00,0xC0,0x4F,0xB6,0xBD,0x3D);
DEFINE_GUID(IID_IAMLatency,0x62EA93BA,0xEC62,0x11D2,0xB7,0x70,0x00,0xC0,0x4F,0xB6,0xBD,0x3D);
DEFINE_GUID(IID_IAMOpenProgress,0x8E1C39A1,0xDE53,0x11CF,0xAA,0x63,0x00,0x80,0xC7,0x44,0x52,0x8D);
DEFINE_GUID(IID_IAMOverlayFX,0x62FAE250,0x7E65,0x4460,0xBF,0xC9,0x63,0x98,0xB3,0x22,0x07,0x3C);
DEFINE_GUID(IID_IAMovie,0x359ACE10,0x7688,0x11CF,0x8B,0x23,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IAMovieSetup,0xA3D8CEC0,0x7E5A,0x11CF,0xBB,0xC5,0x00,0x80,0x5F,0x6C,0xEF,0x20);
DEFINE_GUID(IID_IAMPhysicalPinInfo,0xF938C991,0x3029,0x11CF,0x8C,0x44,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMPushSource,0xF185FE76,0xE64E,0x11D2,0xB7,0x6E,0x00,0xC0,0x4F,0xB6,0xBD,0x3D);
DEFINE_GUID(IID_IAMResourceControl,0x8389D2D0,0x77D7,0x11D1,0xAB,0xE6,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IAMStreamConfig,0xC6E13340,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMStreamControl,0x36B73881,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IAMStreamSelect,0xC1960960,0x17F5,0x11D1,0xAB,0xE1,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IAMTimecodeDisplay,0x9B496CE2,0x811B,0x11CF,0x8C,0x77,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMTimecodeGenerator,0x9B496CE0,0x811B,0x11CF,0x8C,0x77,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMTimecodeReader,0x9B496CE1,0x811B,0x11CF,0x8C,0x77,0x00,0xAA,0x00,0x6B,0x68,0x14);
DEFINE_GUID(IID_IAMTuner,0x211A8761,0x03AC,0x11D1,0x8D,0x13,0x00,0xAA,0x00,0xBD,0x83,0x39);
DEFINE_GUID(IID_IAMTunerNotification,0x211A8760,0x03AC,0x11D1,0x8D,0x13,0x00,0xAA,0x00,0xBD,0x83,0x39);
DEFINE_GUID(IID_IAMTVAudio,0x83EC1C30,0x23D1,0x11D1,0x99,0xE6,0x00,0xA0,0xC9,0x56,0x02,0x66);
DEFINE_GUID(IID_IAMTVAudioNotification,0x83EC1C33,0x23D1,0x11D1,0x99,0xE6,0x00,0xA0,0xC9,0x56,0x02,0x66);
DEFINE_GUID(IID_IAMTVTuner,0x211A8766,0x03AC,0x11D1,0x8D,0x13,0x00,0xAA,0x00,0xBD,0x83,0x39);
DEFINE_GUID(IID_IAMVfwCaptureDialogs,0xD8D715A0,0x6E5E,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_IAMVfwCompressDialogs,0xD8D715A3,0x6E5E,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_IAMVideoCompression,0xC6E13343,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMVideoControl,0x6A2E0670,0x28E4,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAMVideoDecimationProperties,0x60D32930,0x13DA,0x11D3,0x9E,0xC6,0xC4,0xFC,0xAE,0xF5,0xC7,0xBE);
DEFINE_GUID(IID_IAMVideoProcAmp,0xC6E13360,0x30AC,0x11D0,0xA1,0x8C,0x00,0xA0,0xC9,0x11,0x89,0x56);
DEFINE_GUID(IID_IAsyncReader,0x56A868AA,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaFilter,0x56A86899,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IBaseFilter,0x56A86895,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IBPCSatelliteTuner,0x211A8765,0x03AC,0x11D1,0x8D,0x13,0x00,0xAA,0x00,0xBD,0x83,0x39);
DEFINE_GUID(IID_ICaptureGraphBuilder,0xBF87B6E0,0x8C27,0x11D0,0xB3,0xF0,0x00,0xAA,0x00,0x37,0x61,0xC5);
DEFINE_GUID(IID_ICaptureGraphBuilder2,0x93E5A4E0,0x2D50,0x11D2,0xAB,0xFA,0x00,0xA0,0xC9,0xC6,0xE3,0x8D);
DEFINE_GUID(IID_IConfigAviMux,0x5ACD6AA0,0xF482,0x11CE,0x8B,0x67,0x00,0xAA,0x00,0xA3,0xF1,0xA6);
DEFINE_GUID(IID_IConfigInterleaving,0xBEE3D220,0x157B,0x11D0,0xBD,0x23,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_ICreateDevEnum,0x29840822,0x5B84,0x11D0,0xBD,0x3B,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IDDrawExclModeVideo,0x153ACC21,0xD83B,0x11D1,0x82,0xBF,0x00,0xA0,0xC9,0x69,0x6C,0x8F);
DEFINE_GUID(IID_IDDrawExclModeVideoCallback,0x913C24A0,0x20AB,0x11D2,0x90,0x38,0x00,0xA0,0xC9,0x69,0x72,0x98);
DEFINE_GUID(IID_IDecimateVideoImage,0x2E5EA3E0,0xE924,0x11D2,0xB6,0xDA,0x00,0xA0,0xC9,0x95,0xE8,0xDF);
DEFINE_GUID(IID_IDistributorNotify,0x56A868AF,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IDrawVideoImage,0x48EFB120,0xAB49,0x11D2,0xAE,0xD2,0x00,0xA0,0xC9,0x95,0xE8,0xD5);
DEFINE_GUID(IID_IDvdCmd,0x5A4A97E4,0x94EE,0x4A55,0x97,0x51,0x74,0xB5,0x64,0x3A,0xA2,0x7D);
DEFINE_GUID(IID_IDvdControl,0xA70EFE61,0xE2A3,0x11D0,0xA9,0xBE,0x00,0xAA,0x00,0x61,0xBE,0x93);
DEFINE_GUID(IID_IDvdControl2,0x33BC7430,0xEEC0,0x11D2,0x82,0x01,0x00,0xA0,0xC9,0xD7,0x48,0x42);
DEFINE_GUID(IID_IDvdGraphBuilder,0xFCC152B6,0xF372,0x11D0,0x8E,0x00,0x00,0xC0,0x4F,0xD7,0xC0,0x8B);
DEFINE_GUID(IID_IDvdInfo,0xA70EFE60,0xE2A3,0x11D0,0xA9,0xBE,0x00,0xAA,0x00,0x61,0xBE,0x93);
DEFINE_GUID(IID_IDvdInfo2,0x34151510,0xEEC0,0x11D2,0x82,0x01,0x00,0xA0,0xC9,0xD7,0x48,0x42);
DEFINE_GUID(IID_IDvdState,0x86303D6D,0x1C4A,0x4087,0xAB,0x42,0xF7,0x11,0x16,0x70,0x48,0xEF);
DEFINE_GUID(IID_IDVEnc,0xD18E17A0,0xAACB,0x11D0,0xAF,0xB0,0x00,0xAA,0x00,0xB6,0x7A,0x42);
DEFINE_GUID(IID_IDVSplitter,0x92A3A302,0xDA7C,0x4A1F,0xBA,0x7E,0x18,0x02,0xBB,0x5D,0x2D,0x02);
DEFINE_GUID(IID_IEnumFilters,0x56A86893,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IEnumMediaTypes,0x89C31040,0x846B,0x11CE,0x97,0xD3,0x00,0xAA,0x00,0x55,0x59,0x5A);
DEFINE_GUID(IID_IEnumPins,0x56A86892,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IEnumRegFilters,0x56A868A4,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IEnumStreamIdMap,0x945C1566,0x6202,0x46FC,0x96,0xC7,0xD8,0x7F,0x28,0x9C,0x65,0x34);
DEFINE_GUID(IID_IFileSinkFilter,0xA2104830,0x7C70,0x11CF,0x8B,0xCE,0x00,0xAA,0x00,0xA3,0xF1,0xA6);
DEFINE_GUID(IID_IFileSinkFilter2,0x00855B90,0xCE1B,0x11D0,0xBD,0x4F,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IFileSourceFilter,0x56A868A6,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IFilterChain,0xDCFBDCF6,0x0DC2,0x45F5,0x9A,0xB2,0x7C,0x33,0x0E,0xA0,0x9C,0x29);
DEFINE_GUID(IID_IFilterGraph,0x56A8689F,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IFilterGraph2,0x36B73882,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IFilterMapper,0x56A868A3,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IFilterMapper2,0xB79BB0B0,0x33C1,0x11D1,0xAB,0xE1,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IFilterMapper3,0xB79BB0B1,0x33C1,0x11D1,0xAB,0xE1,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IGraphBuilder,0x56A868A9,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IGraphConfig,0x03A1EB8E,0x32BF,0x4245,0x85,0x02,0x11,0x4D,0x08,0xA9,0xCB,0x88);
DEFINE_GUID(IID_IGraphConfigCallback,0xADE0FD60,0xD19D,0x11D2,0xAB,0xF6,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IGraphVersion,0x56A868AB,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IIPDVDec,0xB8E8BD60,0x0BFE,0x11D0,0xAF,0x91,0x00,0xAA,0x00,0xB6,0x7A,0x42);
DEFINE_GUID(IID_IMediaEventSink,0x56A868A2,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaPropertyBag,0x6025A880,0xC0D5,0x11D0,0xBD,0x4E,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IMediaSample,0x56A8689A,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaSample2,0x36B73884,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IMediaSeeking,0x36B73880,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IMemAllocator,0x56A8689C,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMemAllocatorCallbackTemp,0x379A0CF0,0xC1DE,0x11D2,0xAB,0xF5,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IMemAllocatorNotifyCallbackTemp,0x92980B30,0xC1DE,0x11D2,0xAB,0xF5,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IMemInputPin,0x56A8689D,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMpeg2Demultiplexer,0x436EEE9C,0x264F,0x4242,0x90,0xE1,0x4E,0x33,0x0C,0x10,0x75,0x12);
DEFINE_GUID(IID_IMPEG2StreamIdMap,0xD0E04C47,0x25B8,0x4369,0x92,0x5A,0x36,0x2A,0x01,0xD9,0x54,0x44);
DEFINE_GUID(IID_IOverlay,0x56A868A1,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IOverlayNotify,0x56A868A0,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IOverlayNotify2,0x680EFA10,0xD535,0x11D1,0x87,0xC8,0x00,0xA0,0xC9,0x22,0x31,0x96);
DEFINE_GUID(IID_IPersistMediaPropertyBag,0x5738E040,0xB67F,0x11D0,0xBD,0x4D,0x00,0xA0,0xC9,0x11,0xCE,0x86);
DEFINE_GUID(IID_IPin,0x56A86891,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IPinConnection,0x4A9A62D3,0x27D4,0x403D,0x91,0xE9,0x89,0xF5,0x40,0xE5,0x55,0x34);
DEFINE_GUID(IID_IPinFlowControl,0xC56E9858,0xDBF3,0x4F6B,0x81,0x19,0x38,0x4A,0xF2,0x06,0x0D,0xEB);
DEFINE_GUID(IID_IQualityControl,0x56A868A5,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IReferenceClock,0x56A86897,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IReferenceClock2,0x36B73885,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IResourceConsumer,0x56A868AD,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IResourceManager,0x56A868AC,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_ISeekingPassThru,0x36B73883,0xC2C8,0x11CF,0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60);
DEFINE_GUID(IID_IStreamBuilder,0x56A868BF,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IVideoFrameStep,0xE46A9787,0x2B71,0x444D,0xA4,0xB5,0x1F,0xAB,0x7B,0x70,0x8D,0x6A);


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

typedef enum
{
	Famine = 0,
	Flood = 1,
} QualityMessageType;

typedef enum
{
	REG_PINFLAG_B_ZERO = 0x1,
	REG_PINFLAG_B_RENDERER = 0x2,
	REG_PINFLAG_B_MANY = 0x4,
	REG_PINFLAG_B_OUTPUT = 0x8,
} REG_PINFLAG;

typedef enum
{
	AM_SAMPLE_SPLICEPOINT		= 0x1,
	AM_SAMPLE_PREROLL		= 0x2,
	AM_SAMPLE_DATADISCONTINUITY	= 0x4,
	AM_SAMPLE_TYPECHANGED		= 0x8,
	AM_SAMPLE_TIMEVALID		= 0x10,
	AM_SAMPLE_TIMEDISCONTINUITY	= 0x40,
	AM_SAMPLE_FLUSH_ON_PAUSE	= 0x80,
	AM_SAMPLE_STOPVALID		= 0x100,
	AM_SAMPLE_ENDOFSTREAM		= 0x200,

	AM_STREAM_MEDIA			= 0,
	AM_STREAM_CONTROL		= 1
} AM_SAMPLE_PROPERTY_FLAGS;


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

typedef struct
{
	QualityMessageType	Type;
	long			Proportion;
	REFERENCE_TIME		Late;
	REFERENCE_TIME		TimeStamp;
} Quality;

typedef struct
{
	const CLSID*	clsMajorType;
	const CLSID*	clsMinorType;
} REGPINTYPES;

typedef struct
{
	LPWSTR	strName;
	BOOL	bRendered;
	BOOL	bOutput;
	BOOL	bZero;
	BOOL	bMany;
	const CLSID*	clsConnectsToFilter;
	const WCHAR*	strConnectsToPin;
	UINT	nMediaTypes;
	const REGPINTYPES*	lpMediaType;
} REGFILTERPINS;

typedef struct
{
	CLSID	clsMedium;
	DWORD	dw1;
	DWORD	dw2;
} REGPINMEDIUM;

typedef struct
{
	DWORD	dwFlags;
	UINT	cInstances;
	UINT	nMediaTypes;
	const REGPINTYPES*	lpMediaType;
	UINT	nMediums;
	const REGPINMEDIUM*	lpMedium;
	const CLSID*	clsPinCategory;
} REGFILTERPINS2;

typedef struct
{
	DWORD	dwVersion;
	DWORD	dwMerit;
	union {
		struct {
			ULONG	cPins;
			const REGFILTERPINS*	rgPins;
		} DUMMYSTRUCTNAME1;
		struct {
			ULONG	cPins2;
			const REGFILTERPINS2*	rgPins2;
		} DUMMYSTRUCTNAME2;
	} DUMMYUNIONNAME;
} REGFILTER2;

typedef struct
{
	DWORD		cbData;
	DWORD		dwTypeSpecificFlags;
	DWORD		dwSampleFlags;
	LONG		lActual;
	REFERENCE_TIME	tStart;
	REFERENCE_TIME	tStop;
	DWORD		dwStreamId;
	AM_MEDIA_TYPE*	pMediaType;
	BYTE*		pbBuffer;
	LONG		cbBuffer;
} AM_SAMPLE2_PROPERTIES;



/* defines. */

#define CHARS_IN_GUID	39
#define MAX_PIN_NAME	128
#define MAX_FILTER_NAME	128

#define AM_GBF_PREVFRAMESKIPPED		1
#define AM_GBF_NOTASYNCPOINT		2
#define AM_GBF_NOWAIT			4


/* interfaces. */

/**************************************************************************
 *
 * IAsyncReader interface
 *
 */

#define ICOM_INTERFACE IAsyncReader
#define IAsyncReader_METHODS \
    ICOM_METHOD3(HRESULT,RequestAllocator,IMemAllocator*,a1,ALLOCATOR_PROPERTIES*,a2,IMemAllocator**,a3) \
    ICOM_METHOD2(HRESULT,Request,IMediaSample*,a1,DWORD_PTR,a2) \
    ICOM_METHOD3(HRESULT,WaitForNext,DWORD,a1,IMediaSample**,a2,DWORD_PTR*,a3) \
    ICOM_METHOD1(HRESULT,SyncReadAligned,IMediaSample*,a1) \
    ICOM_METHOD3(HRESULT,SyncRead,LONGLONG,a1,LONG,a2,BYTE*,a3) \
    ICOM_METHOD2(HRESULT,Length,LONGLONG*,a1,LONGLONG*,a2) \
    ICOM_METHOD (HRESULT,BeginFlush) \
    ICOM_METHOD (HRESULT,EndFlush)

#define IAsyncReader_IMETHODS \
    IUnknown_IMETHODS \
    IAsyncReader_METHODS

ICOM_DEFINE(IAsyncReader,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IAsyncReader_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IAsyncReader_AddRef(p) ICOM_CALL (AddRef,p)
#define IAsyncReader_Release(p) ICOM_CALL (Release,p)
    /*** IAsyncReader methods ***/
#define IAsyncReader_RequestAllocator(p,a1,a2,a3) ICOM_CALL3(RequestAllocator,p,a1,a2,a3)
#define IAsyncReader_Request(p,a1,a2) ICOM_CALL2(Request,p,a1,a2)
#define IAsyncReader_WaitForNext(p,a1,a2,a3) ICOM_CALL3(WaitForNext,p,a1,a2,a3)
#define IAsyncReader_SyncReadAligned(p,a1) ICOM_CALL1(SyncReadAligned,p,a1)
#define IAsyncReader_SyncRead(p,a1,a2,a3) ICOM_CALL3(SyncRead,p,a1,a2,a3)
#define IAsyncReader_Length(p,a1,a2) ICOM_CALL2(Length,p,a1,a2)
#define IAsyncReader_BeginFlush(p) ICOM_CALL (BeginFlush,p)
#define IAsyncReader_EndFlush(p) ICOM_CALL (EndFlush,p)

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
#define IBaseFilter_Stop(p) ICOM_CALL (Stop,p)
#define IBaseFilter_Pause(p) ICOM_CALL (Pause,p)
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
 * ICaptureGraphBuilder interface
 *
 */

#define ICOM_INTERFACE ICaptureGraphBuilder
#define ICaptureGraphBuilder_METHODS \
    ICOM_METHOD1(HRESULT,SetFiltergraph,IGraphBuilder*,a1) \
    ICOM_METHOD1(HRESULT,GetFiltergraph,IGraphBuilder**,a1) \
    ICOM_METHOD4(HRESULT,SetOutputFileName,const GUID*,a1,LPCOLESTR,a2,IBaseFilter**,a3,IFileSinkFilter**,a4) \
    ICOM_METHOD4(HRESULT,FindInterface,const GUID*,a1,IBaseFilter*,a2,REFIID,a3,void**,a4) \
    ICOM_METHOD4(HRESULT,RenderStream,const GUID*,a1,IUnknown*,a2,IBaseFilter*,a3,IBaseFilter*,a4) \
    ICOM_METHOD6(HRESULT,ControlStream,const GUID*,a1,IBaseFilter*,a2,REFERENCE_TIME*,a3,REFERENCE_TIME*,a4,WORD,a5,WORD,a6) \
    ICOM_METHOD2(HRESULT,AllocCapFile,LPCOLESTR,a1,DWORDLONG,a2) \
    ICOM_METHOD4(HRESULT,CopyCaptureFile,LPOLESTR,a1,LPOLESTR,a2,int,a3,IAMCopyCaptureFileProgress*,a4)

#define ICaptureGraphBuilder_IMETHODS \
    IUnknown_IMETHODS \
    ICaptureGraphBuilder_METHODS

ICOM_DEFINE(ICaptureGraphBuilder,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define ICaptureGraphBuilder_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define ICaptureGraphBuilder_AddRef(p) ICOM_CALL (AddRef,p)
#define ICaptureGraphBuilder_Release(p) ICOM_CALL (Release,p)
    /*** ICaptureGraphBuilder methods ***/
#define ICaptureGraphBuilder_SetFiltergraph(p,a1) ICOM_CALL1(SetFiltergraph,p,a1)
#define ICaptureGraphBuilder_GetFiltergraph(p,a1) ICOM_CALL1(GetFiltergraph,p,a1)
#define ICaptureGraphBuilder_SetOutputFileName(p,a1,a2,a3,a4) ICOM_CALL4(SetOutputFileName,p,a1,a2,a3,a4)
#define ICaptureGraphBuilder_FindInterface(p,a1,a2,a3,a4) ICOM_CALL4(FindInterface,p,a1,a2,a3,a4)
#define ICaptureGraphBuilder_RenderStream(p,a1,a2,a3,a4) ICOM_CALL4(RenderStream,p,a1,a2,a3,a4)
#define ICaptureGraphBuilder_ControlStream(p,a1,a2,a3,a4,a5,a6) ICOM_CALL6(ControlStream,p,a1,a2,a3,a4,a5,a6)
#define ICaptureGraphBuilder_AllocCapFile(p,a1,a2) ICOM_CALL2(AllocCapFile,p,a1,a2)
#define ICaptureGraphBuilder_CopyCaptureFile(p,a1,a2,a3,a4) ICOM_CALL4(CopyCaptureFile,p,a1,a2,a3,a4)

/**************************************************************************
 *
 * ICaptureGraphBuilder2 interface
 *
 */

#define ICOM_INTERFACE ICaptureGraphBuilder2
#define ICaptureGraphBuilder2_METHODS \
    ICOM_METHOD1(HRESULT,SetFiltergraph,IGraphBuilder*,a1) \
    ICOM_METHOD1(HRESULT,GetFiltergraph,IGraphBuilder**,a1) \
    ICOM_METHOD4(HRESULT,SetOutputFileName,const GUID*,a1,LPCOLESTR,a2,IBaseFilter**,a3,IFileSinkFilter**,a4) \
    ICOM_METHOD5(HRESULT,FindInterface,const GUID*,a1,const GUID*,a2,IBaseFilter*,a3,REFIID,a4,void**,a5) \
    ICOM_METHOD5(HRESULT,RenderStream,const GUID*,a1,const GUID*,a2,IUnknown*,a3,IBaseFilter*,a4,IBaseFilter*,a5) \
    ICOM_METHOD7(HRESULT,ControlStream,const GUID*,a1,const GUID*,a2,IBaseFilter*,a3,REFERENCE_TIME*,a4,REFERENCE_TIME*,a5,WORD,a6,WORD,a7) \
    ICOM_METHOD2(HRESULT,AllocCapFile,LPCOLESTR,a1,DWORDLONG,a2) \
    ICOM_METHOD4(HRESULT,CopyCaptureFile,LPOLESTR,a1,LPOLESTR,a2,int,a3,IAMCopyCaptureFileProgress*,a4) \
    ICOM_METHOD7(HRESULT,FindPin,IUnknown*,a1,PIN_DIRECTION,a2,const GUID*,a3,const GUID*,a4,BOOL,a5,int,a6,IPin**,a7)

#define ICaptureGraphBuilder2_IMETHODS \
    IUnknown_IMETHODS \
    ICaptureGraphBuilder2_METHODS

ICOM_DEFINE(ICaptureGraphBuilder2,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define ICaptureGraphBuilder2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define ICaptureGraphBuilder2_AddRef(p) ICOM_CALL (AddRef,p)
#define ICaptureGraphBuilder2_Release(p) ICOM_CALL (Release,p)
    /*** ICaptureGraphBuilder2 methods ***/
#define ICaptureGraphBuilder2_SetFiltergraph(p,a1) ICOM_CALL1(SetFiltergraph,p,a1)
#define ICaptureGraphBuilder2_GetFiltergraph(p,a1) ICOM_CALL1(GetFiltergraph,p,a1)
#define ICaptureGraphBuilder2_SetOutputFileName(p,a1,a2,a3,a4) ICOM_CALL4(SetOutputFileName,p,a1,a2,a3,a4)
#define ICaptureGraphBuilder2_FindInterface(p,a1,a2,a3,a4,a5) ICOM_CALL5(FindInterface,p,a1,a2,a3,a4,a5)
#define ICaptureGraphBuilder2_RenderStream(p,a1,a2,a3,a4,a5) ICOM_CALL5(RenderStream,p,a1,a2,a3,a4,a5)
#define ICaptureGraphBuilder2_ControlStream(p,a1,a2,a3,a4,a5,a6,a7) ICOM_CALL7(ControlStream,p,a1,a2,a3,a4,a5,a6,a7)
#define ICaptureGraphBuilder2_AllocCapFile(p,a1,a2) ICOM_CALL2(AllocCapFile,p,a1,a2)
#define ICaptureGraphBuilder2_CopyCaptureFile(p,a1,a2,a3,a4) ICOM_CALL4(CopyCaptureFile,p,a1,a2,a3,a4)
#define ICaptureGraphBuilder2_FindPin(p,a1,a2,a3,a4,a5,a6,a7) ICOM_CALL7(FindPin,p,a1,a2,a3,a4,a5,a6,a7)

/**************************************************************************
 *
 * ICreateDevEnum interface
 *
 */

#define ICOM_INTERFACE ICreateDevEnum
#define ICreateDevEnum_METHODS \
    ICOM_METHOD3(HRESULT,CreateClassEnumerator,REFCLSID,a1,IEnumMoniker**,a2,DWORD,a3)

#define ICreateDevEnum_IMETHODS \
    IUnknown_IMETHODS \
    ICreateDevEnum_METHODS

ICOM_DEFINE(ICreateDevEnum,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define ICreateDevEnum_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define ICreateDevEnum_AddRef(p) ICOM_CALL (AddRef,p)
#define ICreateDevEnum_Release(p) ICOM_CALL (Release,p)
    /*** ICreateDevEnum methods ***/
#define ICreateDevEnum_CreateClassEnumerator(p,a1,a2,a3) ICOM_CALL3(CreateClassEnumerator,p,a1,a2,a3)

/**************************************************************************
 *
 * IDistributorNotify interface
 *
 */

#define ICOM_INTERFACE IDistributorNotify
#define IDistributorNotify_METHODS \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD (HRESULT,Pause) \
    ICOM_METHOD1(HRESULT,Run,REFERENCE_TIME,a1) \
    ICOM_METHOD1(HRESULT,SetSyncSource,IReferenceClock*,a1) \
    ICOM_METHOD (HRESULT,NotifyGraphChange)

#define IDistributorNotify_IMETHODS \
    IUnknown_IMETHODS \
    IDistributorNotify_METHODS

ICOM_DEFINE(IDistributorNotify,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDistributorNotify_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IDistributorNotify_AddRef(p) ICOM_CALL (AddRef,p)
#define IDistributorNotify_Release(p) ICOM_CALL (Release,p)
    /*** IDistributorNotify methods ***/
#define IDistributorNotify_Stop(p) ICOM_CALL (Stop,p)
#define IDistributorNotify_Pause(p) ICOM_CALL (Pause,p)
#define IDistributorNotify_Run(p,a1) ICOM_CALL1(Run,p,a1)
#define IDistributorNotify_SetSyncSource(p,a1) ICOM_CALL1(SetSyncSource,p,a1)
#define IDistributorNotify_NotifyGraphChange(p) ICOM_CALL (NotifyGraphChange,p)

/**************************************************************************
 *
 * IDVSplitter interface
 *
 */

#define ICOM_INTERFACE IDVSplitter
#define IDVSplitter_METHODS \
    ICOM_METHOD1(HRESULT,DiscardAlternateVideoFrames,int,a1)

#define IDVSplitter_IMETHODS \
    IUnknown_IMETHODS \
    IDVSplitter_METHODS

ICOM_DEFINE(IDVSplitter,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDVSplitter_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IDVSplitter_AddRef(p) ICOM_CALL (AddRef,p)
#define IDVSplitter_Release(p) ICOM_CALL (Release,p)
    /*** IDVSplitter methods ***/
#define IDVSplitter_DiscardAlternateVideoFrames(p,a1) ICOM_CALL1(DiscardAlternateVideoFrames,p,a1)

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
 * IFileSinkFilter interface
 *
 */

#define ICOM_INTERFACE IFileSinkFilter
#define IFileSinkFilter_METHODS \
    ICOM_METHOD2(HRESULT,SetFileName,LPCOLESTR,a1,const AM_MEDIA_TYPE*,a2) \
    ICOM_METHOD2(HRESULT,GetCurFile,LPOLESTR*,a1,AM_MEDIA_TYPE*,a2)

#define IFileSinkFilter_IMETHODS \
    IUnknown_IMETHODS \
    IFileSinkFilter_METHODS

ICOM_DEFINE(IFileSinkFilter,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFileSinkFilter_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFileSinkFilter_AddRef(p) ICOM_CALL (AddRef,p)
#define IFileSinkFilter_Release(p) ICOM_CALL (Release,p)
    /*** IFileSinkFilter methods ***/
#define IFileSinkFilter_SetFileName(p,a1,a2) ICOM_CALL2(SetFileName,p,a1,a2)
#define IFileSinkFilter_GetCurFile(p,a1,a2) ICOM_CALL2(GetCurFile,p,a1,a2)

/**************************************************************************
 *
 * IFileSinkFilter2 interface
 *
 */

#define ICOM_INTERFACE IFileSinkFilter2
#define IFileSinkFilter2_METHODS \
    ICOM_METHOD1(HRESULT,SetMode,DWORD,a1) \
    ICOM_METHOD1(HRESULT,GetMode,DWORD*,a1)

#define IFileSinkFilter2_IMETHODS \
    IFileSinkFilter_IMETHODS \
    IFileSinkFilter2_METHODS

ICOM_DEFINE(IFileSinkFilter2,IFileSinkFilter)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFileSinkFilter2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFileSinkFilter2_AddRef(p) ICOM_CALL (AddRef,p)
#define IFileSinkFilter2_Release(p) ICOM_CALL (Release,p)
    /*** IFileSinkFilter methods ***/
#define IFileSinkFilter2_SetFileName(p,a1,a2) ICOM_CALL2(SetFileName,p,a1,a2)
#define IFileSinkFilter2_GetCurFile(p,a1,a2) ICOM_CALL2(GetCurFile,p,a1,a2)
    /*** IFileSinkFilter2 methods ***/
#define IFileSinkFilter2_SetMode(p,a1) ICOM_CALL1(SetMode,p,a1)
#define IFileSinkFilter2_GetMode(p,a1) ICOM_CALL1(GetMode,p,a1)

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
 * IFilterChain interface
 *
 */

#define ICOM_INTERFACE IFilterChain
#define IFilterChain_METHODS \
    ICOM_METHOD2(HRESULT,StartChain,IBaseFilter*,a1,IBaseFilter*,a2) \
    ICOM_METHOD2(HRESULT,PauseChain,IBaseFilter*,a1,IBaseFilter*,a2) \
    ICOM_METHOD2(HRESULT,StopChain,IBaseFilter*,a1,IBaseFilter*,a2) \
    ICOM_METHOD2(HRESULT,RemoveChain,IBaseFilter*,a1,IBaseFilter*,a2)

#define IFilterChain_IMETHODS \
    IUnknown_IMETHODS \
    IFilterChain_METHODS

ICOM_DEFINE(IFilterChain,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterChain_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterChain_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterChain_Release(p) ICOM_CALL (Release,p)
    /*** IFilterChain methods ***/
#define IFilterChain_StartChain(p,a1,a2) ICOM_CALL2(StartChain,p,a1,a2)
#define IFilterChain_PauseChain(p,a1,a2) ICOM_CALL2(PauseChain,p,a1,a2)
#define IFilterChain_StopChain(p,a1,a2) ICOM_CALL2(StopChain,p,a1,a2)
#define IFilterChain_RemoveChain(p,a1,a2) ICOM_CALL2(RemoveChain,p,a1,a2)

/**************************************************************************
 *
 * IFilterMapper interface
 *
 */

#define ICOM_INTERFACE IFilterMapper
#define IFilterMapper_METHODS \
    ICOM_METHOD3(HRESULT,RegisterFilter,CLSID,a1,LPCWSTR,a2,DWORD,a3) \
    ICOM_METHOD3(HRESULT,RegisterFilterInstance,CLSID,a1,LPCWSTR,a2,CLSID*,a3) \
    ICOM_METHOD8(HRESULT,RegisterPin,CLSID,a1,LPCWSTR,a2,BOOL,a3,BOOL,a4,BOOL,a5,BOOL,a6,CLSID,a7,LPCWSTR,a8) \
    ICOM_METHOD4(HRESULT,RegisterPinType,CLSID,a1,LPCWSTR,a2,CLSID,a3,CLSID,a4) \
    ICOM_METHOD1(HRESULT,UnregisterFilter,CLSID,a1) \
    ICOM_METHOD1(HRESULT,UnregisterFilterInstance,CLSID,a1) \
    ICOM_METHOD2(HRESULT,UnregisterPin,CLSID,a1,LPCWSTR,a2) \
    ICOM_METHOD9(HRESULT,EnumMatchingFilters,IEnumRegFilters**,a1,DWORD,a2,BOOL,a3,CLSID,a4,CLSID,a5,BOOL,a6,BOOL,a7,CLSID,a8,CLSID,a9)

#define IFilterMapper_IMETHODS \
    IUnknown_IMETHODS \
    IFilterMapper_METHODS

ICOM_DEFINE(IFilterMapper,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterMapper_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterMapper_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterMapper_Release(p) ICOM_CALL (Release,p)
    /*** IFilterMapper methods ***/
#define IFilterMapper_RegisterFilter(p,a1,a2,a3) ICOM_CALL3(RegisterFilter,p,a1,a2,a3)
#define IFilterMapper_RegisterFilterInstance(p,a1,a2,a3) ICOM_CALL3(RegisterFilterInstance,p,a1,a2,a3)
#define IFilterMapper_RegisterPin(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(RegisterPin,p,a1,a2,a3,a4,a5,a6,a7,a8)
#define IFilterMapper_RegisterPinType(p,a1,a2,a3,a4) ICOM_CALL4(RegisterPinType,p,a1,a2,a3,a4)
#define IFilterMapper_UnregisterFilter(p,a1) ICOM_CALL1(UnregisterFilter,p,a1)
#define IFilterMapper_UnregisterFilterInstance(p,a1) ICOM_CALL1(UnregisterFilterInstance,p,a1)
#define IFilterMapper_UnregisterPin(p,a1,a2) ICOM_CALL2(UnregisterPin,p,a1,a2)
#define IFilterMapper_EnumMatchingFilters(p,a1,a2,a3,a4,a5,a6,a7,a8,a9) ICOM_CALL9(EnumMatchingFilters,p,a1,a2,a3,a4,a5,a6,a7,a8,a9)

/**************************************************************************
 *
 * IFilterMapper2 interface
 *
 */

#define ICOM_INTERFACE IFilterMapper2
#define IFilterMapper2_METHODS \
    ICOM_METHOD3(HRESULT,CreateCategory,REFCLSID,a1,DWORD,a2,LPCWSTR,a3) \
    ICOM_METHOD3(HRESULT,UnregisterFilter,const CLSID*,a1,const OLECHAR*,a2,REFCLSID,a3) \
    ICOM_METHOD6(HRESULT,RegisterFilter,REFCLSID,a1,LPCWSTR,a2,IMoniker**,a3,const CLSID*,a4,const OLECHAR*,a5,const REGFILTER2*,a6) \
    ICOM_METHOD15(HRESULT,EnumMatchingFilters,IEnumMoniker**,a1,DWORD,a2,BOOL,a3,DWORD,a4,BOOL,a5,DWORD,a6,const GUID*,a7,const REGPINMEDIUM*,a8,const CLSID*,a9,BOOL,a10,BOOL,a11,DWORD,a12,const GUID*,a13,const REGPINMEDIUM*,a14,const CLSID*,a15)

#define IFilterMapper2_IMETHODS \
    IUnknown_IMETHODS \
    IFilterMapper2_METHODS

ICOM_DEFINE(IFilterMapper2,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterMapper2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterMapper2_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterMapper2_Release(p) ICOM_CALL (Release,p)
    /*** IFilterMapper2 methods ***/
#define IFilterMapper2_CreateCategory(p,a1,a2,a3) ICOM_CALL3(CreateCategory,p,a1,a2,a3)
#define IFilterMapper2_UnregisterFilter(p,a1,a2,a3) ICOM_CALL3(UnregisterFilter,p,a1,a2,a3)
#define IFilterMapper2_RegisterFilter(p,a1,a2,a3,a4,a5,a6) ICOM_CALL6(RegisterFilter,p,a1,a2,a3,a4,a5,a6)
#define IFilterMapper2_EnumMatchingFilters(p,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) ICOM_CALL15(EnumMatchingFilters,p,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)

/**************************************************************************
 *
 * IFilterMapper3 interface
 *
 */

#define ICOM_INTERFACE IFilterMapper3
#define IFilterMapper3_METHODS \
    ICOM_METHOD1(HRESULT,GetICreateDevEnum,ICreateDevEnum**,a1)

#define IFilterMapper3_IMETHODS \
    IFilterMapper2_IMETHODS \
    IFilterMapper3_METHODS

ICOM_DEFINE(IFilterMapper3,IFilterMapper2)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterMapper3_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterMapper3_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterMapper3_Release(p) ICOM_CALL (Release,p)
    /*** IFilterMapper2 methods ***/
#define IFilterMapper3_CreateCategory(p,a1,a2,a3) ICOM_CALL3(CreateCategory,p,a1,a2,a3)
#define IFilterMapper3_UnregisterFilter(p,a1,a2,a3) ICOM_CALL3(UnregisterFilter,p,a1,a2,a3)
#define IFilterMapper3_RegisterFilter(p,a1,a2,a3,a4,a5,a6) ICOM_CALL6(RegisterFilter,p,a1,a2,a3,a4,a5,a6)
#define IFilterMapper3_EnumMatchingFilters(p,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) ICOM_CALL15(EnumMatchingFilters,p,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)
    /*** IFilterMapper3 methods ***/
#define IFilterMapper3_GetICreateDevEnum(p,a1) ICOM_CALL1(GetICreateDevEnum,p,a1)

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
 * IGraphBuilder interface
 *
 */

#define ICOM_INTERFACE IGraphBuilder
#define IGraphBuilder_METHODS \
    ICOM_METHOD2(HRESULT,Connect,IPin*,a1,IPin*,a2) \
    ICOM_METHOD1(HRESULT,Render,IPin*,a1) \
    ICOM_METHOD2(HRESULT,RenderFile,LPCWSTR,a1,LPCWSTR,a2) \
    ICOM_METHOD3(HRESULT,AddSourceFilter,LPCWSTR,a1,LPCWSTR,a2,IBaseFilter**,a3) \
    ICOM_METHOD1(HRESULT,SetLogFile,DWORD_PTR,a1) \
    ICOM_METHOD (HRESULT,Abort) \
    ICOM_METHOD (HRESULT,ShouldOperationContinue)

#define IGraphBuilder_IMETHODS \
    IFilterGraph_IMETHODS \
    IGraphBuilder_METHODS

ICOM_DEFINE(IGraphBuilder,IFilterGraph)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IGraphBuilder_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IGraphBuilder_AddRef(p) ICOM_CALL (AddRef,p)
#define IGraphBuilder_Release(p) ICOM_CALL (Release,p)
    /*** IFilterGraph methods ***/
#define IGraphBuilder_AddFilter(p,a1,a2) ICOM_CALL2(AddFilter,p,a1,a2)
#define IGraphBuilder_RemoveFilter(p,a1) ICOM_CALL1(RemoveFilter,p,a1)
#define IGraphBuilder_EnumFilters(p,a1) ICOM_CALL1(EnumFilters,p,a1)
#define IGraphBuilder_FindFilterByName(p,a1,a2) ICOM_CALL2(FindFilterByName,p,a1,a2)
#define IGraphBuilder_ConnectDirect(p,a1,a2,a3) ICOM_CALL3(ConnectDirect,p,a1,a2,a3)
#define IGraphBuilder_Reconnect(p,a1) ICOM_CALL1(Reconnect,p,a1)
#define IGraphBuilder_Disconnect(p,a1) ICOM_CALL1(Disconnect,p,a1)
#define IGraphBuilder_SetDefaultSyncSource(p,a1) ICOM_CALL1(SetDefaultSyncSource,p,a1)
    /*** IGraphBuilder methods ***/
#define IGraphBuilder_Connect(p,a1,a2) ICOM_CALL2(Connect,p,a1,a2)
#define IGraphBuilder_Render(p,a1) ICOM_CALL1(Render,p,a1)
#define IGraphBuilder_RenderFile(p,a1,a2) ICOM_CALL2(RenderFile,p,a1,a2)
#define IGraphBuilder_AddSourceFilter(p,a1,a2,a3) ICOM_CALL3(AddSourceFilter,p,a1,a2,a3)
#define IGraphBuilder_SetLogFile(p,a1) ICOM_CALL1(SetLogFile,p,a1)
#define IGraphBuilder_Abort(p) ICOM_CALL (Abort,p)
#define IGraphBuilder_ShouldOperationContinue(p) ICOM_CALL (ShouldOperationContinue,p)

/**************************************************************************
 *
 * IFilterGraph2 interface
 *
 */

#define ICOM_INTERFACE IFilterGraph2
#define IFilterGraph2_METHODS \
    ICOM_METHOD4(HRESULT,AddSourceFilterForMoniker,IMoniker*,a1,IBindCtx*,a2,LPCWSTR,a3,IBaseFilter**,a4) \
    ICOM_METHOD2(HRESULT,ReconnectEx,IPin*,a1,const AM_MEDIA_TYPE*,a2) \
    ICOM_METHOD3(HRESULT,RenderEx,IPin*,a1,DWORD,a2,DWORD*,a3)

#define IFilterGraph2_IMETHODS \
    IGraphBuilder_IMETHODS \
    IFilterGraph2_METHODS

ICOM_DEFINE(IFilterGraph2,IGraphBuilder)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterGraph2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterGraph2_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterGraph2_Release(p) ICOM_CALL (Release,p)
    /*** IFilterGraph methods ***/
#define IFilterGraph2_AddFilter(p,a1,a2) ICOM_CALL2(AddFilter,p,a1,a2)
#define IFilterGraph2_RemoveFilter(p,a1) ICOM_CALL1(RemoveFilter,p,a1)
#define IFilterGraph2_EnumFilters(p,a1) ICOM_CALL1(EnumFilters,p,a1)
#define IFilterGraph2_FindFilterByName(p,a1,a2) ICOM_CALL2(FindFilterByName,p,a1,a2)
#define IFilterGraph2_ConnectDirect(p,a1,a2,a3) ICOM_CALL3(ConnectDirect,p,a1,a2,a3)
#define IFilterGraph2_Reconnect(p,a1) ICOM_CALL1(Reconnect,p,a1)
#define IFilterGraph2_Disconnect(p,a1) ICOM_CALL1(Disconnect,p,a1)
#define IFilterGraph2_SetDefaultSyncSource(p,a1) ICOM_CALL1(SetDefaultSyncSource,p,a1)
    /*** IGraphBuilder methods ***/
#define IFilterGraph2_Connect(p,a1,a2) ICOM_CALL2(Connect,p,a1,a2)
#define IFilterGraph2_Render(p,a1) ICOM_CALL1(Render,p,a1)
#define IFilterGraph2_RenderFile(p,a1,a2) ICOM_CALL2(RenderFile,p,a1,a2)
#define IFilterGraph2_AddSourceFilter(p,a1,a2,a3) ICOM_CALL3(AddSourceFilter,p,a1,a2,a3)
#define IFilterGraph2_SetLogFile(p,a1) ICOM_CALL1(SetLogFile,p,a1)
#define IFilterGraph2_Abort(p) ICOM_CALL (Abort,p)
#define IFilterGraph2_ShouldOperationContinue(p) ICOM_CALL (ShouldOperationContinue,p)
    /*** IFilterGraph2 methods ***/
#define IFilterGraph2_AddSourceFilterForMoniker(p,a1,a2,a3,a4) ICOM_CALL4(AddSourceFilterForMoniker,p,a1,a2,a3,a4)
#define IFilterGraph2_ReconnectEx(p,a1,a2) ICOM_CALL2(ReconnectEx,p,a1,a2)
#define IFilterGraph2_RenderEx(p,a1,a2,a3) ICOM_CALL3(RenderEx,p,a1,a2,a3)

/**************************************************************************
 *
 * IGraphConfig interface
 *
 */

#define ICOM_INTERFACE IGraphConfig
#define IGraphConfig_METHODS \
    ICOM_METHOD6(HRESULT,Reconnect,IPin*,a1,IPin*,a2,const AM_MEDIA_TYPE*,a3,IBaseFilter*,a4,HANDLE,a5,DWORD,a6) \
    ICOM_METHOD4(HRESULT,Reconfigure,IGraphConfigCallback*,a1,PVOID,a2,DWORD,a3,HANDLE,a4) \
    ICOM_METHOD1(HRESULT,AddFilterToCache,IBaseFilter*,a1) \
    ICOM_METHOD1(HRESULT,EnumCacheFilter,IEnumFilters**,a1) \
    ICOM_METHOD1(HRESULT,RemoveFilterFromCache,IBaseFilter*,a1) \
    ICOM_METHOD1(HRESULT,GetStartTime,REFERENCE_TIME*,a1) \
    ICOM_METHOD3(HRESULT,PushThroughData,IPin*,a1,IPinConnection*,a2,HANDLE,a3) \
    ICOM_METHOD2(HRESULT,SetFilterFlags,IBaseFilter*,a1,DWORD,a2) \
    ICOM_METHOD2(HRESULT,GetFilterFlags,IBaseFilter*,a1,DWORD*,a2) \
    ICOM_METHOD2(HRESULT,RemoveFilterEx,IBaseFilter*,a1,DWORD,a2)

#define IGraphConfig_IMETHODS \
    IUnknown_IMETHODS \
    IGraphConfig_METHODS

ICOM_DEFINE(IGraphConfig,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IGraphConfig_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IGraphConfig_AddRef(p) ICOM_CALL (AddRef,p)
#define IGraphConfig_Release(p) ICOM_CALL (Release,p)
    /*** IGraphConfig methods ***/
#define IGraphConfig_Reconnect(p,a1,a2,a3,a4,a5,a6) ICOM_CALL6(Reconnect,p,a1,a2,a3,a4,a5,a6)
#define IGraphConfig_Reconfigure(p,a1,a2,a3,a4) ICOM_CALL4(Reconfigure,p,a1,a2,a3,a4)
#define IGraphConfig_AddFilterToCache(p,a1) ICOM_CALL1(AddFilterToCache,p,a1)
#define IGraphConfig_EnumCacheFilter(p,a1) ICOM_CALL1(EnumCacheFilter,p,a1)
#define IGraphConfig_RemoveFilterFromCache(p,a1) ICOM_CALL1(RemoveFilterFromCache,p,a1)
#define IGraphConfig_GetStartTime(p,a1) ICOM_CALL1(GetStartTime,p,a1)
#define IGraphConfig_PushThroughData(p,a1,a2,a3) ICOM_CALL3(PushThroughData,p,a1,a2,a3)
#define IGraphConfig_SetFilterFlags(p,a1,a2) ICOM_CALL2(SetFilterFlags,p,a1,a2)
#define IGraphConfig_GetFilterFlags(p,a1,a2) ICOM_CALL2(GetFilterFlags,p,a1,a2)
#define IGraphConfig_RemoveFilterEx(p,a1,a2) ICOM_CALL2(RemoveFilterEx,p,a1,a2)

/**************************************************************************
 *
 * IGraphConfigCallback interface
 *
 */

#define ICOM_INTERFACE IGraphConfigCallback
#define IGraphConfigCallback_METHODS \
    ICOM_METHOD2(HRESULT,Reconfigure,PVOID,a1,DWORD,a2)

#define IGraphConfigCallback_IMETHODS \
    IUnknown_IMETHODS \
    IGraphConfigCallback_METHODS

ICOM_DEFINE(IGraphConfigCallback,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IGraphConfigCallback_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IGraphConfigCallback_AddRef(p) ICOM_CALL (AddRef,p)
#define IGraphConfigCallback_Release(p) ICOM_CALL (Release,p)
    /*** IGraphConfigCallback methods ***/
#define IGraphConfigCallback_Reconfigure(p,a1,a2) ICOM_CALL2(Reconfigure,p,a1,a2)

/**************************************************************************
 *
 * IGraphVersion interface
 *
 */

#define ICOM_INTERFACE IGraphVersion
#define IGraphVersion_METHODS \
    ICOM_METHOD1(HRESULT,QueryVersion,LONG*,a1)

#define IGraphVersion_IMETHODS \
    IUnknown_IMETHODS \
    IGraphVersion_METHODS

ICOM_DEFINE(IGraphVersion,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IGraphVersion_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IGraphVersion_AddRef(p) ICOM_CALL (AddRef,p)
#define IGraphVersion_Release(p) ICOM_CALL (Release,p)
    /*** IGraphVersion methods ***/
#define IGraphVersion_QueryVersion(p,a1) ICOM_CALL1(QueryVersion,p,a1)


/**************************************************************************
 *
 * IMediaEventSink interface
 *
 */

#define ICOM_INTERFACE IMediaEventSink
#define IMediaEventSink_METHODS \
    ICOM_METHOD3(HRESULT,Notify,long,a1,LONG_PTR,a2,LONG_PTR,a3)

#define IMediaEventSink_IMETHODS \
    IUnknown_IMETHODS \
    IMediaEventSink_METHODS

ICOM_DEFINE(IMediaEventSink,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaEventSink_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaEventSink_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaEventSink_Release(p) ICOM_CALL (Release,p)
    /*** IMediaEventSink methods ***/
#define IMediaEventSink_Notify(p,a1,a2,a3) ICOM_CALL3(Notify,p,a1,a2,a3)

/**************************************************************************
 *
 * IMediaPropertyBag interface
 *
 */

#define ICOM_INTERFACE IMediaPropertyBag
#define IMediaPropertyBag_METHODS \
    ICOM_METHOD3(HRESULT,EnumProperty,ULONG,a1,VARIANT*,a2,VARIANT*,a3)

#define IMediaPropertyBag_IMETHODS \
    IPropertyBag_IMETHODS \
    IMediaPropertyBag_METHODS

ICOM_DEFINE(IMediaPropertyBag,IPropertyBag)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaPropertyBag_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaPropertyBag_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaPropertyBag_Release(p) ICOM_CALL (Release,p)
    /*** IPropertyBag methods ***/
#define IMediaPropertyBag_Read(p,a1,a2,a3) ICOM_CALL3(Read,p,a1,a2,a3)
#define IMediaPropertyBag_Write(p,a1,a2) ICOM_CALL2(Write,p,a1,a2)
    /*** IMediaPropertyBag methods ***/
#define IMediaPropertyBag_EnumProperty(p,a1,a2,a3) ICOM_CALL3(EnumProperty,p,a1,a2,a3)

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
#define IMediaSample_GetActualDataLength(p) ICOM_CALL (GetActualDataLength,p)
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
 * IMediaSeeking interface
 *
 */

#define ICOM_INTERFACE IMediaSeeking
#define IMediaSeeking_METHODS \
    ICOM_METHOD1(HRESULT,GetCapabilities,DWORD*,a1) \
    ICOM_METHOD1(HRESULT,CheckCapabilities,DWORD*,a1) \
    ICOM_METHOD1(HRESULT,IsFormatSupported,const GUID*,a1) \
    ICOM_METHOD1(HRESULT,QueryPreferredFormat,GUID*,a1) \
    ICOM_METHOD1(HRESULT,GetTimeFormat,GUID*,a1) \
    ICOM_METHOD1(HRESULT,IsUsingTimeFormat,const GUID*,a1) \
    ICOM_METHOD1(HRESULT,SetTimeFormat,const GUID*,a1) \
    ICOM_METHOD1(HRESULT,GetDuration,LONGLONG*,a1) \
    ICOM_METHOD1(HRESULT,GetStopPosition,LONGLONG*,a1) \
    ICOM_METHOD1(HRESULT,GetCurrentPosition,LONGLONG*,a1) \
    ICOM_METHOD4(HRESULT,ConvertTimeFormat,LONGLONG*,a1,const GUID*,a2,LONGLONG,a3,const GUID*,a4) \
    ICOM_METHOD4(HRESULT,SetPositions,LONGLONG*,a1,DWORD,a2,LONGLONG*,a3,DWORD,a4) \
    ICOM_METHOD2(HRESULT,GetPositions,LONGLONG*,a1,LONGLONG*,a2) \
    ICOM_METHOD2(HRESULT,GetAvailable,LONGLONG*,a1,LONGLONG*,a2) \
    ICOM_METHOD1(HRESULT,SetRate,double,a1) \
    ICOM_METHOD1(HRESULT,GetRate,double*,a1) \
    ICOM_METHOD1(HRESULT,GetPreroll,LONGLONG*,a1)

#define IMediaSeeking_IMETHODS \
    IUnknown_IMETHODS \
    IMediaSeeking_METHODS

ICOM_DEFINE(IMediaSeeking,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaSeeking_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaSeeking_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaSeeking_Release(p) ICOM_CALL (Release,p)
    /*** IMediaSeeking methods ***/
#define IMediaSeeking_GetCapabilities(p,a1) ICOM_CALL1(GetCapabilities,p,a1)
#define IMediaSeeking_CheckCapabilities(p,a1) ICOM_CALL1(CheckCapabilities,p,a1)
#define IMediaSeeking_IsFormatSupported(p,a1) ICOM_CALL1(IsFormatSupported,p,a1)
#define IMediaSeeking_QueryPreferredFormat(p,a1) ICOM_CALL1(QueryPreferredFormat,p,a1)
#define IMediaSeeking_GetTimeFormat(p,a1) ICOM_CALL1(GetTimeFormat,p,a1)
#define IMediaSeeking_IsUsingTimeFormat(p,a1) ICOM_CALL1(IsUsingTimeFormat,p,a1)
#define IMediaSeeking_SetTimeFormat(p,a1) ICOM_CALL1(SetTimeFormat,p,a1)
#define IMediaSeeking_GetDuration(p,a1) ICOM_CALL1(GetDuration,p,a1)
#define IMediaSeeking_GetStopPosition(p,a1) ICOM_CALL1(GetStopPosition,p,a1)
#define IMediaSeeking_GetCurrentPosition(p,a1) ICOM_CALL1(GetCurrentPosition,p,a1)
#define IMediaSeeking_ConvertTimeFormat(p,a1,a2,a3,a4) ICOM_CALL4(ConvertTimeFormat,p,a1,a2,a3,a4)
#define IMediaSeeking_SetPositions(p,a1,a2,a3,a4) ICOM_CALL4(SetPositions,p,a1,a2,a3,a4)
#define IMediaSeeking_GetPositions(p,a1,a2) ICOM_CALL2(GetPositions,p,a1,a2)
#define IMediaSeeking_GetAvailable(p,a1,a2) ICOM_CALL2(GetAvailable,p,a1,a2)
#define IMediaSeeking_SetRate(p,a1) ICOM_CALL1(SetRate,p,a1)
#define IMediaSeeking_GetRate(p,a1) ICOM_CALL1(GetRate,p,a1)
#define IMediaSeeking_GetPreroll(p,a1) ICOM_CALL1(GetPreroll,p,a1)

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
 * IOverlay interface
 *
 */

#define ICOM_INTERFACE IOverlay
#define IOverlay_METHODS \
    ICOM_METHOD2(HRESULT,GetPalette,DWORD*,a1,PALETTEENTRY**,a2) \
    ICOM_METHOD2(HRESULT,SetPalette,DWORD,a1,PALETTEENTRY*,a2) \
    ICOM_METHOD1(HRESULT,GetDefaultColorKey,COLORKEY*,a1) \
    ICOM_METHOD1(HRESULT,GetColorKey,COLORKEY*,a1) \
    ICOM_METHOD1(HRESULT,SetColorKey,COLORKEY*,a1) \
    ICOM_METHOD1(HRESULT,GetWindowHandle,HWND*,a1) \
    ICOM_METHOD3(HRESULT,GetClipList,RECT*,a1,RECT*,a2,RGNDATA**,a3) \
    ICOM_METHOD2(HRESULT,GetVideoPosition,RECT*,a1,RECT*,a2) \
    ICOM_METHOD2(HRESULT,Advise,IOverlayNotify*,a1,DWORD,a2) \
    ICOM_METHOD (HRESULT,Unadvise)

#define IOverlay_IMETHODS \
    IUnknown_IMETHODS \
    IOverlay_METHODS

ICOM_DEFINE(IOverlay,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IOverlay_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IOverlay_AddRef(p) ICOM_CALL (AddRef,p)
#define IOverlay_Release(p) ICOM_CALL (Release,p)
    /*** IOverlay methods ***/
#define IOverlay_GetPalette(p,a1,a2) ICOM_CALL2(GetPalette,p,a1,a2)
#define IOverlay_SetPalette(p,a1,a2) ICOM_CALL2(SetPalette,p,a1,a2)
#define IOverlay_GetDefaultColorKey(p,a1) ICOM_CALL1(GetDefaultColorKey,p,a1)
#define IOverlay_GetColorKey(p,a1) ICOM_CALL1(GetColorKey,p,a1)
#define IOverlay_SetColorKey(p,a1) ICOM_CALL1(SetColorKey,p,a1)
#define IOverlay_GetWindowHandle(p,a1) ICOM_CALL1(GetWindowHandle,p,a1)
#define IOverlay_GetClipList(p,a1,a2,a3) ICOM_CALL3(GetClipList,p,a1,a2,a3)
#define IOverlay_GetVideoPosition(p,a1,a2) ICOM_CALL2(GetVideoPosition,p,a1,a2)
#define IOverlay_Advise(p,a1,a2) ICOM_CALL2(Advise,p,a1,a2)
#define IOverlay_Unadvise(p) ICOM_CALL1(Unadvise,p)

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
 * IQualityControl interface
 *
 */

#define ICOM_INTERFACE IQualityControl
#define IQualityControl_METHODS \
    ICOM_METHOD2(HRESULT,Notify,IBaseFilter*,a1,Quality,a2) \
    ICOM_METHOD1(HRESULT,SetSink,IQualityControl*,a1)

#define IQualityControl_IMETHODS \
    IUnknown_IMETHODS \
    IQualityControl_METHODS

ICOM_DEFINE(IQualityControl,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IQualityControl_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IQualityControl_AddRef(p) ICOM_CALL (AddRef,p)
#define IQualityControl_Release(p) ICOM_CALL (Release,p)
    /*** IQualityControl methods ***/
#define IQualityControl_Notify(p,a1,a2) ICOM_CALL2(Notify,p,a1,a2)
#define IQualityControl_SetSink(p,a1) ICOM_CALL1(SetSink,p,a1)

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

/**************************************************************************
 *
 * ISeekingPassThru interface
 *
 */

#define ICOM_INTERFACE ISeekingPassThru
#define ISeekingPassThru_METHODS \
    ICOM_METHOD2(HRESULT,Init,BOOL,a1,IPin*,a2)

#define ISeekingPassThru_IMETHODS \
    IUnknown_IMETHODS \
    ISeekingPassThru_METHODS

ICOM_DEFINE(ISeekingPassThru,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define ISeekingPassThru_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define ISeekingPassThru_AddRef(p) ICOM_CALL (AddRef,p)
#define ISeekingPassThru_Release(p) ICOM_CALL (Release,p)
    /*** ISeekingPassThru methods ***/
#define ISeekingPassThru_Init(p,a1,a2) ICOM_CALL2(Init,p,a1,a2)




#endif  /* __WINE_STRMIF_H_ */
