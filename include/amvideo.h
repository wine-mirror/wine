#ifndef __WINE_AMVIDEO_H_
#define __WINE_AMVIDEO_H_

#include "ole2.h"
#include "ddraw.h"


typedef struct IBaseVideoMixer IBaseVideoMixer;
typedef struct IDirectDrawVideo IDirectDrawVideo;
typedef struct IFullScreenVideo IFullScreenVideo;
typedef struct IFullScreenVideoEx IFullScreenVideoEx;
typedef struct IQualProp IQualProp;


#define	iEGA_COLORS	16
#define	iPALETTE_COLORS	256
#define	iMASK_COLORS	3
#define	iRED	0
#define	iGREEN	1
#define	iBLUE	2

#define WIDTHBYTES(bits)	((DWORD)((((DWORD)(bits)+31U)&(~31U))>>3))
#define DIBWIDTHBYTES(bi)	((DWORD)WIDTHBYTES((bi).biWidth*(bi).biBitCount))
#define DIBSIZE(bi)	(DIBWIDTHBYTES(bi)*(DWORD)abs((bi).biHeight))


typedef struct
{
	DWORD	dwBitMasks[iMASK_COLORS];
	RGBQUAD	bmiColors[iPALETTE_COLORS];
} TRUECOLORINFO;

typedef struct
{
	RECT	rcSource;
	RECT	rcTarget;
	DWORD	dwBitRate;
	DWORD	dwBitErrorRate;
	REFERENCE_TIME	AvgTimePerFrame;
	BITMAPINFOHEADER	bmiHeader;
} VIDEOINFOHEADER;

typedef struct
{
	RECT	rcSource;
	RECT	rcTarget;
	DWORD	dwBitRate;
	DWORD	dwBitErrorRate;
	REFERENCE_TIME	AvgTimePerFrame;
	BITMAPINFOHEADER	bmiHeader;

    union {
		RGBQUAD	bmiColors[iPALETTE_COLORS];
		DWORD	dwBitMasks[iMASK_COLORS];
		TRUECOLORINFO	TrueColorInfo;
    } DUMMYUNIONNAME;
} VIDEOINFO;

typedef struct
{
	VIDEOINFOHEADER	hdr;
	DWORD	dwStartTimeCode;
	DWORD	cbSequenceHeader;
	BYTE	bSequenceHeader[1];
} MPEG1VIDEOINFO;

typedef struct
{
	RECT	rcSource;
	RECT	rcTarget;
	DWORD	dwActiveWidth;
	DWORD	dwActiveHeight;
	REFERENCE_TIME	AvgTimePerFrame;
} ANALOGVIDEOINFO;


/**************************************************************************
 *
 * IBaseVideoMixer interface
 *
 */

#define ICOM_INTERFACE IBaseVideoMixer
#define IBaseVideoMixer_METHODS \
    ICOM_METHOD1(HRESULT,SetLeadPin,int,a1) \
    ICOM_METHOD1(HRESULT,GetLeadPin,int*,a1) \
    ICOM_METHOD1(HRESULT,GetInputPinCount,int*,a1) \
    ICOM_METHOD1(HRESULT,IsUsingClock,int*,a1) \
    ICOM_METHOD1(HRESULT,SetUsingClock,int,a1) \
    ICOM_METHOD1(HRESULT,GetClockPeriod,int*,a1) \
    ICOM_METHOD1(HRESULT,SetClockPeriod,int,a1)

#define IBaseVideoMixer_IMETHODS \
    IUnknown_IMETHODS \
    IBaseVideoMixer_METHODS

ICOM_DEFINE(IBaseVideoMixer,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IBaseVideoMixer_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IBaseVideoMixer_AddRef(p) ICOM_CALL (AddRef,p)
#define IBaseVideoMixer_Release(p) ICOM_CALL (Release,p)
    /*** IBaseVideoMixer methods ***/
#define IBaseVideoMixer_SetLeadPin(p,a1) ICOM_CALL1(SetLeadPin,p,a1)
#define IBaseVideoMixer_GetLeadPin(p,a1) ICOM_CALL1(GetLeadPin,p,a1)
#define IBaseVideoMixer_GetInputPinCount(p,a1) ICOM_CALL1(GetInputPinCount,p,a1)
#define IBaseVideoMixer_IsUsingClock(p,a1) ICOM_CALL1(IsUsingClock,p,a1)
#define IBaseVideoMixer_SetUsingClock(p,a1) ICOM_CALL1(SetUsingClock,p,a1)
#define IBaseVideoMixer_GetClockPeriod(p,a1) ICOM_CALL1(GetClockPeriod,p,a1)
#define IBaseVideoMixer_SetClockPeriod(p,a1) ICOM_CALL1(SetClockPeriod,p,a1)

/**************************************************************************
 *
 * IDirectDrawVideo interface
 *
 */

#define ICOM_INTERFACE IDirectDrawVideo
#define IDirectDrawVideo_METHODS \
    ICOM_METHOD1(HRESULT,GetSwitches,DWORD*,a1) \
    ICOM_METHOD1(HRESULT,SetSwitches,DWORD,a1) \
    ICOM_METHOD1(HRESULT,GetCaps,DDCAPS*,a1) \
    ICOM_METHOD1(HRESULT,GetEmulatedCaps,DDCAPS*,a1) \
    ICOM_METHOD1(HRESULT,GetSurfaceDesc,DDSURFACEDESC*,a1) \
    ICOM_METHOD2(HRESULT,GetFourCCCodes,DWORD*,a1,DWORD*,a2) \
    ICOM_METHOD1(HRESULT,SetDirectDraw,LPDIRECTDRAW,a1) \
    ICOM_METHOD1(HRESULT,GetDirectDraw,LPDIRECTDRAW*,a1) \
    ICOM_METHOD1(HRESULT,GetSurfaceType,DWORD*,a1) \
    ICOM_METHOD(HRESULT,SetDefault) \
    ICOM_METHOD1(HRESULT,UseScanLine,long,a1) \
    ICOM_METHOD1(HRESULT,CanUseScanLine,long*,a1) \
    ICOM_METHOD1(HRESULT,UseOverlayStretch,long,a1) \
    ICOM_METHOD1(HRESULT,CanUseOverlayStretch,long*,a1) \
    ICOM_METHOD1(HRESULT,UseWhenFullScreen,long,a1) \
    ICOM_METHOD1(HRESULT,WillUseFullScreen,long*,a1)

#define IDirectDrawVideo_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawVideo_METHODS

ICOM_DEFINE(IDirectDrawVideo,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDirectDrawVideo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IDirectDrawVideo_AddRef(p) ICOM_CALL (AddRef,p)
#define IDirectDrawVideo_Release(p) ICOM_CALL (Release,p)
    /*** IDirectDrawVideo methods ***/
#define IDirectDrawVideo_GetSwitches(p,a1) ICOM_CALL1(GetSwitches,p,a1)
#define IDirectDrawVideo_SetSwitches(p,a1) ICOM_CALL1(SetSwitches,p,a1)
#define IDirectDrawVideo_GetCaps(p,a1) ICOM_CALL1(GetCaps,p,a1)
#define IDirectDrawVideo_GetEmulatedCaps(p,a1) ICOM_CALL1(GetEmulatedCaps,p,a1)
#define IDirectDrawVideo_GetSurfaceDesc(p,a1) ICOM_CALL1(GetSurfaceDesc,p,a1)
#define IDirectDrawVideo_GetFourCCCodes(p,a1,a2) ICOM_CALL2(GetFourCCCodes,p,a1,a2)
#define IDirectDrawVideo_SetDirectDraw(p,a1) ICOM_CALL1(SetDirectDraw,p,a1)
#define IDirectDrawVideo_GetDirectDraw(p,a1) ICOM_CALL1(GetDirectDraw,p,a1)
#define IDirectDrawVideo_GetSurfaceType(p,a1) ICOM_CALL1(GetSurfaceType,p,a1)
#define IDirectDrawVideo_SetDefault(p) ICOM_CALL (SetDefault,p)
#define IDirectDrawVideo_UseScanLine(p,a1) ICOM_CALL1(UseScanLine,p,a1)
#define IDirectDrawVideo_CanUseScanLine(p,a1) ICOM_CALL1(CanUseScanLine,p,a1)
#define IDirectDrawVideo_UseOverlayStretch(p,a1) ICOM_CALL1(UseOverlayStretch,p,a1)
#define IDirectDrawVideo_CanUseOverlayStretch(p,a1) ICOM_CALL1(CanUseOverlayStretch,p,a1)
#define IDirectDrawVideo_UseWhenFullScreen(p,a1) ICOM_CALL1(UseWhenFullScreen,p,a1)
#define IDirectDrawVideo_WillUseFullScreen(p,a1) ICOM_CALL1(WillUseFullScreen,p,a1)

/**************************************************************************
 *
 * IFullScreenVideo interface
 *
 */

#define ICOM_INTERFACE IFullScreenVideo
#define IFullScreenVideo_METHODS \
    ICOM_METHOD1(HRESULT,CountModes,long*,a1) \
    ICOM_METHOD4(HRESULT,GetModeInfo,long,a1,long*,a2,long*,a3,long*,a4) \
    ICOM_METHOD1(HRESULT,GetCurrentMode,long*,a1) \
    ICOM_METHOD1(HRESULT,IsModeAvailable,long,a1) \
    ICOM_METHOD1(HRESULT,IsModeEnabled,long,a1) \
    ICOM_METHOD2(HRESULT,SetEnabled,long,a1,long,a2) \
    ICOM_METHOD1(HRESULT,GetClipFactor,long*,a1) \
    ICOM_METHOD1(HRESULT,SetClipFactor,long,a1) \
    ICOM_METHOD1(HRESULT,SetMessageDrain,HWND,a1) \
    ICOM_METHOD1(HRESULT,GetMessageDrain,HWND*,a1) \
    ICOM_METHOD1(HRESULT,SetMonitor,long,a1) \
    ICOM_METHOD1(HRESULT,GetMonitor,long*,a1) \
    ICOM_METHOD1(HRESULT,HideOnDeactivate,long,a1) \
    ICOM_METHOD(HRESULT,IsHideOnDeactivate) \
    ICOM_METHOD1(HRESULT,SetCaption,BSTR,a1) \
    ICOM_METHOD1(HRESULT,GetCaption,BSTR*,a1) \
    ICOM_METHOD(HRESULT,SetDefault)

#define IFullScreenVideo_IMETHODS \
    IUnknown_IMETHODS \
    IFullScreenVideo_METHODS

ICOM_DEFINE(IFullScreenVideo,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFullScreenVideo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFullScreenVideo_AddRef(p) ICOM_CALL (AddRef,p)
#define IFullScreenVideo_Release(p) ICOM_CALL (Release,p)
    /*** IFullScreenVideo methods ***/
#define IFullScreenVideo_CountModes(p,a1) ICOM_CALL1(CountModes,p,a1)
#define IFullScreenVideo_GetModeInfo(p,a1,a2,a3,a4) ICOM_CALL4(GetModeInfo,p,a1,a2,a3,a4)
#define IFullScreenVideo_GetCurrentMode(p,a1) ICOM_CALL1(GetCurrentMode,p,a1)
#define IFullScreenVideo_IsModeAvailable(p,a1) ICOM_CALL1(IsModeAvailable,p,a1)
#define IFullScreenVideo_IsModeEnabled(p,a1) ICOM_CALL1(IsModeEnabled,p,a1)
#define IFullScreenVideo_SetEnabled(p,a1,a2) ICOM_CALL2(SetEnabled,p,a1,a2)
#define IFullScreenVideo_GetClipFactor(p,a1) ICOM_CALL1(GetClipFactor,p,a1)
#define IFullScreenVideo_SetClipFactor(p,a1) ICOM_CALL1(SetClipFactor,p,a1)
#define IFullScreenVideo_SetMessageDrain(p,a1) ICOM_CALL1(SetMessageDrain,p,a1)
#define IFullScreenVideo_GetMessageDrain(p,a1) ICOM_CALL1(GetMessageDrain,p,a1)
#define IFullScreenVideo_SetMonitor(p,a1) ICOM_CALL1(SetMonitor,p,a1)
#define IFullScreenVideo_GetMonitor(p,a1) ICOM_CALL1(GetMonitor,p,a1)
#define IFullScreenVideo_HideOnDeactivate(p,a1) ICOM_CALL1(HideOnDeactivate,p,a1)
#define IFullScreenVideo_IsHideOnDeactivate(p) ICOM_CALL (IsHideOnDeactivate,p)
#define IFullScreenVideo_SetCaption(p,a1) ICOM_CALL1(SetCaption,p,a1)
#define IFullScreenVideo_GetCaption(p,a1) ICOM_CALL1(GetCaption,p,a1)
#define IFullScreenVideo_SetDefault(p) ICOM_CALL (SetDefault,p)

/**************************************************************************
 *
 * IFullScreenVideoEx interface
 *
 */

#define ICOM_INTERFACE IFullScreenVideoEx
#define IFullScreenVideoEx_METHODS \
    ICOM_METHOD2(HRESULT,SetAcceleratorTable,HWND,a1,HACCEL,a2) \
    ICOM_METHOD2(HRESULT,GetAcceleratorTable,HWND*,a1,HACCEL*,a2) \
    ICOM_METHOD1(HRESULT,KeepPixelAspectRatio,long,a1) \
    ICOM_METHOD1(HRESULT,IsKeepPixelAspectRatio,long*,a1)

#define IFullScreenVideoEx_IMETHODS \
    IFullScreenVideo_IMETHODS \
    IFullScreenVideoEx_METHODS

ICOM_DEFINE(IFullScreenVideoEx,IFullScreenVideo)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFullScreenVideoEx_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFullScreenVideoEx_AddRef(p) ICOM_CALL (AddRef,p)
#define IFullScreenVideoEx_Release(p) ICOM_CALL (Release,p)
    /*** IFullScreenVideo methods ***/
#define IFullScreenVideoEx_CountModes(p,a1) ICOM_CALL1(CountModes,p,a1)
#define IFullScreenVideoEx_GetModeInfo(p,a1,a2,a3,a4) ICOM_CALL4(GetModeInfo,p,a1,a2,a3,a4)
#define IFullScreenVideoEx_GetCurrentMode(p,a1) ICOM_CALL1(GetCurrentMode,p,a1)
#define IFullScreenVideoEx_IsModeAvailable(p,a1) ICOM_CALL1(IsModeAvailable,p,a1)
#define IFullScreenVideoEx_IsModeEnabled(p,a1) ICOM_CALL1(IsModeEnabled,p,a1)
#define IFullScreenVideoEx_SetEnabled(p,a1,a2) ICOM_CALL2(SetEnabled,p,a1,a2)
#define IFullScreenVideoEx_GetClipFactor(p,a1) ICOM_CALL1(GetClipFactor,p,a1)
#define IFullScreenVideoEx_SetClipFactor(p,a1) ICOM_CALL1(SetClipFactor,p,a1)
#define IFullScreenVideoEx_SetMessageDrain(p,a1) ICOM_CALL1(SetMessageDrain,p,a1)
#define IFullScreenVideoEx_GetMessageDrain(p,a1) ICOM_CALL1(GetMessageDrain,p,a1)
#define IFullScreenVideoEx_SetMonitor(p,a1) ICOM_CALL1(SetMonitor,p,a1)
#define IFullScreenVideoEx_GetMonitor(p,a1) ICOM_CALL1(GetMonitor,p,a1)
#define IFullScreenVideoEx_HideOnDeactivate(p,a1) ICOM_CALL1(HideOnDeactivate,p,a1)
#define IFullScreenVideoEx_IsHideOnDeactivate(p) ICOM_CALL (IsHideOnDeactivate,p)
#define IFullScreenVideoEx_SetCaption(p,a1) ICOM_CALL1(SetCaption,p,a1)
#define IFullScreenVideoEx_GetCaption(p,a1) ICOM_CALL1(GetCaption,p,a1)
#define IFullScreenVideoEx_SetDefault(p) ICOM_CALL (SetDefault,p)
    /*** IFullScreenVideoEx methods ***/
#define IFullScreenVideoEx_SetAcceleratorTable(p,a1,a2) ICOM_CALL2(SetAcceleratorTable,p,a1,a2)
#define IFullScreenVideoEx_GetAcceleratorTable(p,a1,a2) ICOM_CALL2(GetAcceleratorTable,p,a1,a2)
#define IFullScreenVideoEx_KeepPixelAspectRatio(p,a1) ICOM_CALL1(KeepPixelAspectRatio,p,a1)
#define IFullScreenVideoEx_IsKeepPixelAspectRatio(p,a1) ICOM_CALL1(IsKeepPixelAspectRatio,p,a1)

/**************************************************************************
 *
 * IQualProp interface
 *
 */

#define ICOM_INTERFACE IQualProp
#define IQualProp_METHODS \
    ICOM_METHOD1(HRESULT,get_FramesDroppedInRenderer,int*,a1) \
    ICOM_METHOD1(HRESULT,get_FramesDrawn,int*,a1) \
    ICOM_METHOD1(HRESULT,get_AvgFrameRate,int*,a1) \
    ICOM_METHOD1(HRESULT,get_Jitter,int*,a1) \
    ICOM_METHOD1(HRESULT,get_AvgSyncOffset,int*,a1) \
    ICOM_METHOD1(HRESULT,get_DevSyncOffset,int*,a1)

#define IQualProp_IMETHODS \
    IUnknown_IMETHODS \
    IQualProp_METHODS

ICOM_DEFINE(IQualProp,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IQualProp_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IQualProp_AddRef(p) ICOM_CALL (AddRef,p)
#define IQualProp_Release(p) ICOM_CALL (Release,p)
    /*** IQualProp methods ***/
#define IQualProp_get_FramesDroppedInRenderer(p,a1) ICOM_CALL1(get_FramesDroppedInRenderer,p,a1)
#define IQualProp_get_FramesDrawn(p,a1) ICOM_CALL1(get_FramesDrawn,p,a1)
#define IQualProp_get_AvgFrameRate(p,a1) ICOM_CALL1(get_AvgFrameRate,p,a1)
#define IQualProp_get_Jitter(p,a1) ICOM_CALL1(get_Jitter,p,a1)
#define IQualProp_get_AvgSyncOffset(p,a1) ICOM_CALL1(get_AvgSyncOffset,p,a1)
#define IQualProp_get_DevSyncOffset(p,a1) ICOM_CALL1(get_DevSyncOffset,p,a1)


#endif  /* __WINE_AMVIDEO_H_ */
