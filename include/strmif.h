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
