#ifndef __WINE_CONTROL_H_
#define __WINE_CONTROL_H_

#include "ole2.h"

/* forward decls. */

typedef struct IAMCollection IAMCollection;
typedef struct IAMStats IAMStats;
typedef struct IBasicAudio IBasicAudio;
typedef struct IBasicVideo IBasicVideo;
typedef struct IBasicVideo2 IBasicVideo2;
typedef struct IDeferredCommand IDeferredCommand;
typedef struct IFilterInfo IFilterInfo;
typedef struct IMediaControl IMediaControl;
typedef struct IMediaEvent IMediaEvent;
typedef struct IMediaEventEx IMediaEventEx;
typedef struct IMediaPosition IMediaPosition;
typedef struct IMediaTypeInfo IMediaTypeInfo;
typedef struct IPinInfo IPinInfo;
typedef struct IQueueCommand IQueueCommand;
typedef struct IRegFilterInfo IRegFilterInfo;
typedef struct IVideoWindow IVideoWindow;

/* GUIDs */

DEFINE_GUID(IID_IAMCollection,0x56A868B9,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IAMStats,0xBC9BCF80,0xDCD2,0x11D2,0xAB,0xF6,0x00,0xA0,0xC9,0x05,0xF3,0x75);
DEFINE_GUID(IID_IBasicAudio,0x56A868B3,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IBasicVideo,0x56A868B5,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IBasicVideo2,0x329BB360,0xF6EA,0x11D1,0x90,0x38,0x00,0xA0,0xC9,0x69,0x72,0x98);
DEFINE_GUID(IID_IDeferredCommand,0x56A868B8,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IFilterInfo,0x56A868BA,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaControl,0x56A868B1,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaEvent,0x56A868B6,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaEventEx,0x56A868C0,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaPosition,0x56A868B2,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IMediaTypeInfo,0x56A868BC,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IPinInfo,0x56A868BD,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IQueueCommand,0x56A868B7,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IRegFilterInfo,0x56A868BB,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);
DEFINE_GUID(IID_IVideoWindow,0x56A868B4,0x0AD4,0x11CE,0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70);

#ifndef __WINE_REFTIME_DEFINED_
#define __WINE_REFTIME_DEFINED_
typedef double REFTIME;
#endif  /* __WINE_REFTIME_DEFINED_ */

typedef LONG_PTR OAEVENT;
typedef LONG_PTR OAHWND;
typedef long OAFilterState;



/**************************************************************************
 *
 * IAMCollection interface
 *
 */

#define ICOM_INTERFACE IAMCollection
#define IAMCollection_METHODS \
    ICOM_METHOD1(HRESULT,get_Count,LONG*,a1) \
    ICOM_METHOD2(HRESULT,Item,long,a1,IUnknown**,a2) \
    ICOM_METHOD1(HRESULT,get__NewEnum,IUnknown**,a1)

#define IAMCollection_IMETHODS \
    IDispatch_IMETHODS \
    IAMCollection_METHODS

ICOM_DEFINE(IAMCollection,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IAMCollection_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IAMCollection_AddRef(p) ICOM_CALL (AddRef,p)
#define IAMCollection_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IAMCollection_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IAMCollection_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IAMCollection_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IAMCollection_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IAMCollection methods ***/
#define IAMCollection_get_Count(p,a1) ICOM_CALL1(get_Count,p,a1)
#define IAMCollection_Item(p,a1,a2) ICOM_CALL2(Item,p,a1,a2)
#define IAMCollection_get__NewEnum(p,a1) ICOM_CALL1(get__NewEnum,p,a1)

/**************************************************************************
 *
 * IAMStats interface
 *
 */

#define ICOM_INTERFACE IAMStats
#define IAMStats_METHODS \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,get_Count,LONG*,a1) \
    ICOM_METHOD8(HRESULT,GetValueByIndex,long,a1,BSTR*,a2,long*,a3,double*,a4,double*,a5,double*,a6,double*,a7,double*,a8) \
    ICOM_METHOD8(HRESULT,GetValueByName,BSTR,a1,long*,a2,long*,a3,double*,a4,double*,a5,double*,a6,double*,a7,double*,a8) \
    ICOM_METHOD3(HRESULT,GetIndex,BSTR,a1,long,a2,long*,a3) \
    ICOM_METHOD2(HRESULT,AddValue,long,a1,double,a2)

#define IAMStats_IMETHODS \
    IDispatch_IMETHODS \
    IAMStats_METHODS

ICOM_DEFINE(IAMStats,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IAMStats_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IAMStats_AddRef(p) ICOM_CALL (AddRef,p)
#define IAMStats_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IAMStats_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IAMStats_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IAMStats_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IAMStats_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IAMStats methods ***/
#define IAMStats_Reset(p) ICOM_CALL (Reset,p)
#define IAMStats_get_Count(p,a1) ICOM_CALL1(get_Count,p,a1)
#define IAMStats_GetValueByIndex(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(GetValueByIndex,p,a1,a2,a3,a4,a5,a6,a7,a8)
#define IAMStats_GetValueByName(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(GetValueByName,p,a1,a2,a3,a4,a5,a6,a7,a8)
#define IAMStats_GetIndex(p,a1,a2,a3) ICOM_CALL3(GetIndex,p,a1,a2,a3)
#define IAMStats_AddValue(p,a1,a2) ICOM_CALL2(AddValue,p,a1,a2)

/**************************************************************************
 *
 * IBasicAudio interface
 *
 */

#define ICOM_INTERFACE IBasicAudio
#define IBasicAudio_METHODS \
    ICOM_METHOD1(HRESULT,put_Volume,long,a1) \
    ICOM_METHOD1(HRESULT,get_Volume,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Balance,long,a1) \
    ICOM_METHOD1(HRESULT,get_Balance,long*,a1)

#define IBasicAudio_IMETHODS \
    IDispatch_IMETHODS \
    IBasicAudio_METHODS

ICOM_DEFINE(IBasicAudio,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IBasicAudio_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IBasicAudio_AddRef(p) ICOM_CALL (AddRef,p)
#define IBasicAudio_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IBasicAudio_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IBasicAudio_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IBasicAudio_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IBasicAudio_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IBasicAudio methods ***/
#define IBasicAudio_put_Volume(p,a1) ICOM_CALL1(put_Volume,p,a1)
#define IBasicAudio_get_Volume(p,a1) ICOM_CALL1(get_Volume,p,a1)
#define IBasicAudio_put_Balance(p,a1) ICOM_CALL1(put_Balance,p,a1)
#define IBasicAudio_get_Balance(p,a1) ICOM_CALL1(get_Balance,p,a1)

/**************************************************************************
 *
 * IBasicVideo interface
 *
 */

#define ICOM_INTERFACE IBasicVideo
#define IBasicVideo_METHODS \
    ICOM_METHOD1(HRESULT,get_AvgTimePerFrame,REFTIME*,a1) \
    ICOM_METHOD1(HRESULT,get_BitRate,long*,a1) \
    ICOM_METHOD1(HRESULT,get_BitErrorRate,long*,a1) \
    ICOM_METHOD1(HRESULT,get_VideoWidth,long*,a1) \
    ICOM_METHOD1(HRESULT,get_VideoHeight,long*,a1) \
    ICOM_METHOD1(HRESULT,put_SourceLeft,long,a1) \
    ICOM_METHOD1(HRESULT,get_SourceLeft,long*,a1) \
    ICOM_METHOD1(HRESULT,put_SourceWidth,long,a1) \
    ICOM_METHOD1(HRESULT,get_SourceWidth,long*,a1) \
    ICOM_METHOD1(HRESULT,put_SourceTop,long,a1) \
    ICOM_METHOD1(HRESULT,get_SourceTop,long*,a1) \
    ICOM_METHOD1(HRESULT,put_SourceHeight,long,a1) \
    ICOM_METHOD1(HRESULT,get_SourceHeight,long*,a1) \
    ICOM_METHOD1(HRESULT,put_DestinationLeft,long,a1) \
    ICOM_METHOD1(HRESULT,get_DestinationLeft,long*,a1) \
    ICOM_METHOD1(HRESULT,put_DestinationWidth,long,a1) \
    ICOM_METHOD1(HRESULT,get_DestinationWidth,long*,a1) \
    ICOM_METHOD1(HRESULT,put_DestinationTop,long,a1) \
    ICOM_METHOD1(HRESULT,get_DestinationTop,long*,a1) \
    ICOM_METHOD1(HRESULT,put_DestinationHeight,long,a1) \
    ICOM_METHOD1(HRESULT,get_DestinationHeight,long*,a1) \
    ICOM_METHOD4(HRESULT,SetSourcePosition,long,a1,long,a2,long,a3,long,a4) \
    ICOM_METHOD4(HRESULT,GetSourcePosition,long*,a1,long*,a2,long*,a3,long*,a4) \
    ICOM_METHOD (HRESULT,SetDefaultSourcePosition) \
    ICOM_METHOD4(HRESULT,SetDestinationPosition,long,a1,long,a2,long,a3,long,a4) \
    ICOM_METHOD4(HRESULT,GetDestinationPosition,long*,a1,long*,a2,long*,a3,long*,a4) \
    ICOM_METHOD (HRESULT,SetDefaultDestinationPosition) \
    ICOM_METHOD2(HRESULT,GetVideoSize,long*,a1,long*,a2) \
    ICOM_METHOD4(HRESULT,GetVideoPaletteEntries,long,a1,long,a2,long*,a3,long*,a4) \
    ICOM_METHOD2(HRESULT,GetCurrentImage,long*,a1,long*,a2) \
    ICOM_METHOD (HRESULT,IsUsingDefaultSource) \
    ICOM_METHOD (HRESULT,IsUsingDefaultDestination)

#define IBasicVideo_IMETHODS \
    IDispatch_IMETHODS \
    IBasicVideo_METHODS

ICOM_DEFINE(IBasicVideo,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IBasicVideo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IBasicVideo_AddRef(p) ICOM_CALL (AddRef,p)
#define IBasicVideo_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IBasicVideo_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IBasicVideo_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IBasicVideo_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IBasicVideo_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IBasicVideo methods ***/
#define IBasicVideo_get_AvgTimePerFrame(p,a1) ICOM_CALL1(get_AvgTimePerFrame,p,a1)
#define IBasicVideo_get_BitRate(p,a1) ICOM_CALL1(get_BitRate,p,a1)
#define IBasicVideo_get_BitErrorRate(p,a1) ICOM_CALL1(get_BitErrorRate,p,a1)
#define IBasicVideo_get_VideoWidth(p,a1) ICOM_CALL1(get_VideoWidth,p,a1)
#define IBasicVideo_get_VideoHeight(p,a1) ICOM_CALL1(get_VideoHeight,p,a1)
#define IBasicVideo_put_SourceLeft(p,a1) ICOM_CALL1(put_SourceLeft,p,a1)
#define IBasicVideo_get_SourceLeft(p,a1) ICOM_CALL1(get_SourceLeft,p,a1)
#define IBasicVideo_put_SourceWidth(p,a1) ICOM_CALL1(put_SourceWidth,p,a1)
#define IBasicVideo_get_SourceWidth(p,a1) ICOM_CALL1(get_SourceWidth,p,a1)
#define IBasicVideo_put_SourceTop(p,a1) ICOM_CALL1(put_SourceTop,p,a1)
#define IBasicVideo_get_SourceTop(p,a1) ICOM_CALL1(get_SourceTop,p,a1)
#define IBasicVideo_put_SourceHeight(p,a1) ICOM_CALL1(put_SourceHeight,p,a1)
#define IBasicVideo_get_SourceHeight(p,a1) ICOM_CALL1(get_SourceHeight,p,a1)
#define IBasicVideo_put_DestinationLeft(p,a1) ICOM_CALL1(put_DestinationLeft,p,a1)
#define IBasicVideo_get_DestinationLeft(p,a1) ICOM_CALL1(get_DestinationLeft,p,a1)
#define IBasicVideo_put_DestinationWidth(p,a1) ICOM_CALL1(put_DestinationWidth,p,a1)
#define IBasicVideo_get_DestinationWidth(p,a1) ICOM_CALL1(get_DestinationWidth,p,a1)
#define IBasicVideo_put_DestinationTop(p,a1) ICOM_CALL1(put_DestinationTop,p,a1)
#define IBasicVideo_get_DestinationTop(p,a1) ICOM_CALL1(get_DestinationTop,p,a1)
#define IBasicVideo_put_DestinationHeight(p,a1) ICOM_CALL1(put_DestinationHeight,p,a1)
#define IBasicVideo_get_DestinationHeight(p,a1) ICOM_CALL1(get_DestinationHeight,p,a1)
#define IBasicVideo_SetSourcePosition(p,a1,a2,a3,a4) ICOM_CALL4(SetSourcePosition,p,a1,a2,a3,a4)
#define IBasicVideo_GetSourcePosition(p,a1,a2,a3,a4) ICOM_CALL4(GetSourcePosition,p,a1,a2,a3,a4)
#define IBasicVideo_SetDefaultSourcePosition(p) ICOM_CALL (SetDefaultSourcePosition,p)
#define IBasicVideo_SetDestinationPosition(p,a1,a2,a3,a4) ICOM_CALL4(SetDestinationPosition,p,a1,a2,a3,a4)
#define IBasicVideo_GetDestinationPosition(p,a1,a2,a3,a4) ICOM_CALL4(GetDestinationPosition,p,a1,a2,a3,a4)
#define IBasicVideo_SetDefaultDestinationPosition(p) ICOM_CALL (SetDefaultDestinationPosition,p)
#define IBasicVideo_GetVideoSize(p,a1,a2) ICOM_CALL2(GetVideoSize,p,a1,a2)
#define IBasicVideo_GetVideoPaletteEntries(p,a1,a2,a3,a4) ICOM_CALL4(GetVideoPaletteEntries,p,a1,a2,a3,a4)
#define IBasicVideo_GetCurrentImage(p,a1,a2) ICOM_CALL2(GetCurrentImage,p,a1,a2)
#define IBasicVideo_IsUsingDefaultSource(p) ICOM_CALL (IsUsingDefaultSource,p)
#define IBasicVideo_IsUsingDefaultDestination(p) ICOM_CALL (IsUsingDefaultDestination,p)

/**************************************************************************
 *
 * IBasicVideo2 interface
 *
 */

#define ICOM_INTERFACE IBasicVideo2
#define IBasicVideo2_METHODS \
    ICOM_METHOD2(HRESULT,GetPreferredAspectRatio,long*,a1,long*,a2)

#define IBasicVideo2_IMETHODS \
    IBasicVideo_IMETHODS \
    IBasicVideo2_METHODS

ICOM_DEFINE(IBasicVideo2,IBasicVideo)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IBasicVideo2_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IBasicVideo2_AddRef(p) ICOM_CALL (AddRef,p)
#define IBasicVideo2_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IBasicVideo2_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IBasicVideo2_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IBasicVideo2_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IBasicVideo2_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IBasicVideo methods ***/
#define IBasicVideo2_get_AvgTimePerFrame(p,a1) ICOM_CALL1(get_AvgTimePerFrame,p,a1)
#define IBasicVideo2_get_BitRate(p,a1) ICOM_CALL1(get_BitRate,p,a1)
#define IBasicVideo2_get_BitErrorRate(p,a1) ICOM_CALL1(get_BitErrorRate,p,a1)
#define IBasicVideo2_get_VideoWidth(p,a1) ICOM_CALL1(get_VideoWidth,p,a1)
#define IBasicVideo2_get_VideoHeight(p,a1) ICOM_CALL1(get_VideoHeight,p,a1)
#define IBasicVideo2_put_SourceLeft(p,a1) ICOM_CALL1(put_SourceLeft,p,a1)
#define IBasicVideo2_get_SourceLeft(p,a1) ICOM_CALL1(get_SourceLeft,p,a1)
#define IBasicVideo2_put_SourceWidth(p,a1) ICOM_CALL1(put_SourceWidth,p,a1)
#define IBasicVideo2_get_SourceWidth(p,a1) ICOM_CALL1(get_SourceWidth,p,a1)
#define IBasicVideo2_put_SourceTop(p,a1) ICOM_CALL1(put_SourceTop,p,a1)
#define IBasicVideo2_get_SourceTop(p,a1) ICOM_CALL1(get_SourceTop,p,a1)
#define IBasicVideo2_put_SourceHeight(p,a1) ICOM_CALL1(put_SourceHeight,p,a1)
#define IBasicVideo2_get_SourceHeight(p,a1) ICOM_CALL1(get_SourceHeight,p,a1)
#define IBasicVideo2_put_DestinationLeft(p,a1) ICOM_CALL1(put_DestinationLeft,p,a1)
#define IBasicVideo2_get_DestinationLeft(p,a1) ICOM_CALL1(get_DestinationLeft,p,a1)
#define IBasicVideo2_put_DestinationWidth(p,a1) ICOM_CALL1(put_DestinationWidth,p,a1)
#define IBasicVideo2_get_DestinationWidth(p,a1) ICOM_CALL1(get_DestinationWidth,p,a1)
#define IBasicVideo2_put_DestinationTop(p,a1) ICOM_CALL1(put_DestinationTop,p,a1)
#define IBasicVideo2_get_DestinationTop(p,a1) ICOM_CALL1(get_DestinationTop,p,a1)
#define IBasicVideo2_put_DestinationHeight(p,a1) ICOM_CALL1(put_DestinationHeight,p,a1)
#define IBasicVideo2_get_DestinationHeight(p,a1) ICOM_CALL1(get_DestinationHeight,p,a1)
#define IBasicVideo2_SetSourcePosition(p,a1,a2,a3,a4) ICOM_CALL4(SetSourcePosition,p,a1,a2,a3,a4)
#define IBasicVideo2_GetSourcePosition(p,a1,a2,a3,a4) ICOM_CALL4(GetSourcePosition,p,a1,a2,a3,a4)
#define IBasicVideo2_SetDefaultSourcePosition(p,a1) ICOM_CALL1(SetDefaultSourcePosition,p,a1)
#define IBasicVideo2_SetDestinationPosition(p,a1,a2,a3,a4) ICOM_CALL4(SetDestinationPosition,p,a1,a2,a3,a4)
#define IBasicVideo2_GetDestinationPosition(p,a1,a2,a3,a4) ICOM_CALL4(GetDestinationPosition,p,a1,a2,a3,a4)
#define IBasicVideo2_SetDefaultDestinationPosition(p,a1) ICOM_CALL1(SetDefaultDestinationPosition,p,a1)
#define IBasicVideo2_GetVideoSize(p,a1,a2) ICOM_CALL2(GetVideoSize,p,a1,a2)
#define IBasicVideo2_GetVideoPaletteEntries(p,a1,a2,a3,a4) ICOM_CALL4(GetVideoPaletteEntries,p,a1,a2,a3,a4)
#define IBasicVideo2_GetCurrentImage(p,a1,a2) ICOM_CALL2(GetCurrentImage,p,a1,a2)
#define IBasicVideo2_IsUsingDefaultSource(p,a1) ICOM_CALL1(IsUsingDefaultSource,p,a1)
#define IBasicVideo2_IsUsingDefaultDestination(p,a1) ICOM_CALL1(IsUsingDefaultDestination,p,a1)
    /*** IBasicVideo2 methods ***/
#define IBasicVideo2_GetPreferredAspectRatio(p,a1,a2) ICOM_CALL2(GetPreferredAspectRatio,p,a1,a2)

/**************************************************************************
 *
 * IDeferredCommand interface
 *
 */

#define ICOM_INTERFACE IDeferredCommand
#define IDeferredCommand_METHODS \
    ICOM_METHOD (HRESULT,Cancel) \
    ICOM_METHOD1(HRESULT,Confidence,LONG*,a1) \
    ICOM_METHOD1(HRESULT,Postpone,REFTIME,a1) \
    ICOM_METHOD1(HRESULT,GetHResult,HRESULT*,a1)

#define IDeferredCommand_IMETHODS \
    IUnknown_IMETHODS \
    IDeferredCommand_METHODS

ICOM_DEFINE(IDeferredCommand,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IDeferredCommand_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IDeferredCommand_AddRef(p) ICOM_CALL (AddRef,p)
#define IDeferredCommand_Release(p) ICOM_CALL (Release,p)
    /*** IDeferredCommand methods ***/
#define IDeferredCommand_Cancel(p) ICOM_CALL1(Cancel,p)
#define IDeferredCommand_Confidence(p,a1) ICOM_CALL1(Confidence,p,a1)
#define IDeferredCommand_Postpone(p,a1) ICOM_CALL1(Postpone,p,a1)
#define IDeferredCommand_GetHResult(p,a1) ICOM_CALL1(GetHResult,p,a1)

/**************************************************************************
 *
 * IFilterInfo interface
 *
 */

#define ICOM_INTERFACE IFilterInfo
#define IFilterInfo_METHODS \
    ICOM_METHOD2(HRESULT,FindPin,BSTR,a1,IDispatch**,a2) \
    ICOM_METHOD1(HRESULT,get_Name,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,get_VendorInfo,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,get_Filter,IUnknown**,a1) \
    ICOM_METHOD1(HRESULT,get_Pins,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,get_IsFileSource,LONG*,a1) \
    ICOM_METHOD1(HRESULT,get_Filename,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,put_Filename,BSTR,a1)

#define IFilterInfo_IMETHODS \
    IDispatch_IMETHODS \
    IFilterInfo_METHODS

ICOM_DEFINE(IFilterInfo,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IFilterInfo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IFilterInfo_AddRef(p) ICOM_CALL (AddRef,p)
#define IFilterInfo_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IFilterInfo_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IFilterInfo_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IFilterInfo_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IFilterInfo_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IFilterInfo methods ***/
#define IFilterInfo_FindPin(p,a1,a2) ICOM_CALL2(FindPin,p,a1,a2)
#define IFilterInfo_get_Name(p,a1) ICOM_CALL1(get_Name,p,a1)
#define IFilterInfo_get_VendorInfo(p,a1) ICOM_CALL1(get_VendorInfo,p,a1)
#define IFilterInfo_get_Filter(p,a1) ICOM_CALL1(get_Filter,p,a1)
#define IFilterInfo_get_Pins(p,a1) ICOM_CALL1(get_Pins,p,a1)
#define IFilterInfo_get_IsFileSource(p,a1) ICOM_CALL1(get_IsFileSource,p,a1)
#define IFilterInfo_get_Filename(p,a1) ICOM_CALL1(get_Filename,p,a1)
#define IFilterInfo_put_Filename(p,a1) ICOM_CALL1(put_Filename,p,a1)

/**************************************************************************
 *
 * IMediaControl interface
 *
 */

#define ICOM_INTERFACE IMediaControl
#define IMediaControl_METHODS \
    ICOM_METHOD (HRESULT,Run) \
    ICOM_METHOD (HRESULT,Pause) \
    ICOM_METHOD (HRESULT,Stop) \
    ICOM_METHOD2(HRESULT,GetState,LONG,a1,OAFilterState*,a2) \
    ICOM_METHOD1(HRESULT,RenderFile,BSTR,a1) \
    ICOM_METHOD2(HRESULT,AddSourceFilter,BSTR,a1,IDispatch**,a2) \
    ICOM_METHOD1(HRESULT,get_FilterCollection,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,get_RegFilterCollection,IDispatch**,a1) \
    ICOM_METHOD (HRESULT,StopWhenReady)

#define IMediaControl_IMETHODS \
    IDispatch_IMETHODS \
    IMediaControl_METHODS

ICOM_DEFINE(IMediaControl,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaControl_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaControl_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaControl_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IMediaControl_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IMediaControl_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IMediaControl_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IMediaControl_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IMediaControl methods ***/
#define IMediaControl_Run(p) ICOM_CALL (Run,p)
#define IMediaControl_Pause(p) ICOM_CALL (Pause,p)
#define IMediaControl_Stop(p) ICOM_CALL (Stop,p)
#define IMediaControl_GetState(p,a1,a2) ICOM_CALL2(GetState,p,a1,a2)
#define IMediaControl_RenderFile(p,a1) ICOM_CALL1(RenderFile,p,a1)
#define IMediaControl_AddSourceFilter(p,a1,a2) ICOM_CALL2(AddSourceFilter,p,a1,a2)
#define IMediaControl_get_FilterCollection(p,a1) ICOM_CALL1(get_FilterCollection,p,a1)
#define IMediaControl_get_RegFilterCollection(p,a1) ICOM_CALL1(get_RegFilterCollection,p,a1)
#define IMediaControl_StopWhenReady(p) ICOM_CALL (StopWhenReady,p)

/**************************************************************************
 *
 * IMediaEvent interface
 *
 */

#define ICOM_INTERFACE IMediaEvent
#define IMediaEvent_METHODS \
    ICOM_METHOD1(HRESULT,GetEventHandle,OAEVENT*,a1) \
    ICOM_METHOD4(HRESULT,GetEvent,long*,a1,LONG_PTR*,a2,LONG_PTR*,a3,long,a4) \
    ICOM_METHOD2(HRESULT,WaitForCompletion,long,a1,long*,a2) \
    ICOM_METHOD1(HRESULT,CancelDefaultHandling,long,a1) \
    ICOM_METHOD1(HRESULT,RestoreDefaultHandling,long,a1) \
    ICOM_METHOD3(HRESULT,FreeEventParams,long,a1,LONG_PTR,a2,LONG_PTR,a3)

#define IMediaEvent_IMETHODS \
    IDispatch_IMETHODS \
    IMediaEvent_METHODS

ICOM_DEFINE(IMediaEvent,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaEvent_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaEvent_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaEvent_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IMediaEvent_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IMediaEvent_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IMediaEvent_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IMediaEvent_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IMediaEvent methods ***/
#define IMediaEvent_GetEventHandle(p,a1) ICOM_CALL1(GetEventHandle,p,a1)
#define IMediaEvent_GetEvent(p,a1,a2,a3,a4) ICOM_CALL4(GetEvent,p,a1,a2,a3,a4)
#define IMediaEvent_WaitForCompletion(p,a1,a2) ICOM_CALL2(WaitForCompletion,p,a1,a2)
#define IMediaEvent_CancelDefaultHandling(p,a1) ICOM_CALL1(CancelDefaultHandling,p,a1)
#define IMediaEvent_RestoreDefaultHandling(p,a1) ICOM_CALL1(RestoreDefaultHandling,p,a1)
#define IMediaEvent_FreeEventParams(p,a1,a2,a3) ICOM_CALL3(FreeEventParams,p,a1,a2,a3)

/**************************************************************************
 *
 * IMediaEventEx interface
 *
 */

#define ICOM_INTERFACE IMediaEventEx
#define IMediaEventEx_METHODS \
    ICOM_METHOD3(HRESULT,SetNotifyWindow,OAHWND,a1,long,a2,LONG_PTR,a3) \
    ICOM_METHOD1(HRESULT,SetNotifyFlags,long,a1) \
    ICOM_METHOD1(HRESULT,GetNotifyFlags,long*,a1)

#define IMediaEventEx_IMETHODS \
    IMediaEvent_IMETHODS \
    IMediaEventEx_METHODS

ICOM_DEFINE(IMediaEventEx,IMediaEvent)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaEventEx_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaEventEx_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaEventEx_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IMediaEventEx_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IMediaEventEx_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IMediaEventEx_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IMediaEventEx_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IMediaEvent methods ***/
#define IMediaEventEx_GetEventHandle(p,a1) ICOM_CALL1(GetEventHandle,p,a1)
#define IMediaEventEx_GetEvent(p,a1,a2,a3,a4) ICOM_CALL4(GetEvent,p,a1,a2,a3,a4)
#define IMediaEventEx_WaitForCompletion(p,a1,a2) ICOM_CALL2(WaitForCompletion,p,a1,a2)
#define IMediaEventEx_CancelDefaultHandling(p,a1) ICOM_CALL1(CancelDefaultHandling,p,a1)
#define IMediaEventEx_RestoreDefaultHandling(p,a1) ICOM_CALL1(RestoreDefaultHandling,p,a1)
#define IMediaEventEx_FreeEventParams(p,a1,a2,a3) ICOM_CALL3(FreeEventParams,p,a1,a2,a3)
    /*** IMediaEventEx methods ***/
#define IMediaEventEx_SetNotifyWindow(p,a1,a2,a3) ICOM_CALL3(SetNotifyWindow,p,a1,a2,a3)
#define IMediaEventEx_SetNotifyFlags(p,a1) ICOM_CALL1(SetNotifyFlags,p,a1)
#define IMediaEventEx_GetNotifyFlags(p,a1) ICOM_CALL1(GetNotifyFlags,p,a1)

/**************************************************************************
 *
 * IMediaPosition interface
 *
 */

#define ICOM_INTERFACE IMediaPosition
#define IMediaPosition_METHODS \
    ICOM_METHOD1(HRESULT,get_Duration,REFTIME*,a1) \
    ICOM_METHOD1(HRESULT,put_CurrentPosition,REFTIME,a1) \
    ICOM_METHOD1(HRESULT,get_CurrentPosition,REFTIME*,a1) \
    ICOM_METHOD1(HRESULT,get_StopTime,REFTIME*,a1) \
    ICOM_METHOD1(HRESULT,put_StopTime,REFTIME,a1) \
    ICOM_METHOD1(HRESULT,get_PrerollTime,REFTIME*,a1) \
    ICOM_METHOD1(HRESULT,put_PrerollTime,REFTIME,a1) \
    ICOM_METHOD1(HRESULT,put_Rate,double,a1) \
    ICOM_METHOD1(HRESULT,get_Rate,double*,a1) \
    ICOM_METHOD1(HRESULT,CanSeekForward,LONG*,a1) \
    ICOM_METHOD1(HRESULT,CanSeekBackward,LONG*,a1)

#define IMediaPosition_IMETHODS \
    IDispatch_IMETHODS \
    IMediaPosition_METHODS

ICOM_DEFINE(IMediaPosition,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaPosition_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaPosition_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaPosition_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IMediaPosition_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IMediaPosition_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IMediaPosition_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IMediaPosition_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IMediaPosition methods ***/
#define IMediaPosition_get_Duration(p,a1) ICOM_CALL1(get_Duration,p,a1)
#define IMediaPosition_put_CurrentPosition(p,a1) ICOM_CALL1(put_CurrentPosition,p,a1)
#define IMediaPosition_get_CurrentPosition(p,a1) ICOM_CALL1(get_CurrentPosition,p,a1)
#define IMediaPosition_get_StopTime(p,a1) ICOM_CALL1(get_StopTime,p,a1)
#define IMediaPosition_put_StopTime(p,a1) ICOM_CALL1(put_StopTime,p,a1)
#define IMediaPosition_get_PrerollTime(p,a1) ICOM_CALL1(get_PrerollTime,p,a1)
#define IMediaPosition_put_PrerollTime(p,a1) ICOM_CALL1(put_PrerollTime,p,a1)
#define IMediaPosition_put_Rate(p,a1) ICOM_CALL1(put_Rate,p,a1)
#define IMediaPosition_get_Rate(p,a1) ICOM_CALL1(get_Rate,p,a1)
#define IMediaPosition_CanSeekForward(p,a1) ICOM_CALL1(CanSeekForward,p,a1)
#define IMediaPosition_CanSeekBackward(p,a1) ICOM_CALL1(CanSeekBackward,p,a1)

/**************************************************************************
 *
 * IMediaTypeInfo interface
 *
 */

#define ICOM_INTERFACE IMediaTypeInfo
#define IMediaTypeInfo_METHODS \
    ICOM_METHOD1(HRESULT,get_Type,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,get_Subtype,BSTR*,a1)

#define IMediaTypeInfo_IMETHODS \
    IDispatch_IMETHODS \
    IMediaTypeInfo_METHODS

ICOM_DEFINE(IMediaTypeInfo,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IMediaTypeInfo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IMediaTypeInfo_AddRef(p) ICOM_CALL (AddRef,p)
#define IMediaTypeInfo_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IMediaTypeInfo_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IMediaTypeInfo_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IMediaTypeInfo_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IMediaTypeInfo_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IMediaTypeInfo methods ***/
#define IMediaTypeInfo_get_Type(p,a1) ICOM_CALL1(get_Type,p,a1)
#define IMediaTypeInfo_get_Subtype(p,a1) ICOM_CALL1(get_Subtype,p,a1)

/**************************************************************************
 *
 * IPinInfo interface
 *
 */

#define ICOM_INTERFACE IPinInfo
#define IPinInfo_METHODS \
    ICOM_METHOD1(HRESULT,get_Pin,IUnknown**,a1) \
    ICOM_METHOD1(HRESULT,get_ConnectedTo,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,get_ConnectionMediaType,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,get_FilterInfo,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,get_Name,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,get_Direction,LONG*,a1) \
    ICOM_METHOD1(HRESULT,get_PinID,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,get_MediaTypes,IDispatch**,a1) \
    ICOM_METHOD1(HRESULT,Connect,IUnknown*,a1) \
    ICOM_METHOD1(HRESULT,ConnectDirect,IUnknown*,a1) \
    ICOM_METHOD2(HRESULT,ConnectWithType,IUnknown*,a1,IDispatch*,a2) \
    ICOM_METHOD (HRESULT,Disconnect) \
    ICOM_METHOD (HRESULT,Render)

#define IPinInfo_IMETHODS \
    IDispatch_IMETHODS \
    IPinInfo_METHODS

ICOM_DEFINE(IPinInfo,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IPinInfo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IPinInfo_AddRef(p) ICOM_CALL (AddRef,p)
#define IPinInfo_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IPinInfo_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IPinInfo_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IPinInfo_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IPinInfo_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IPinInfo methods ***/
#define IPinInfo_get_Pin(p,a1) ICOM_CALL1(get_Pin,p,a1)
#define IPinInfo_get_ConnectedTo(p,a1) ICOM_CALL1(get_ConnectedTo,p,a1)
#define IPinInfo_get_ConnectionMediaType(p,a1) ICOM_CALL1(get_ConnectionMediaType,p,a1)
#define IPinInfo_get_FilterInfo(p,a1) ICOM_CALL1(get_FilterInfo,p,a1)
#define IPinInfo_get_Name(p,a1) ICOM_CALL1(get_Name,p,a1)
#define IPinInfo_get_Direction(p,a1) ICOM_CALL1(get_Direction,p,a1)
#define IPinInfo_get_PinID(p,a1) ICOM_CALL1(get_PinID,p,a1)
#define IPinInfo_get_MediaTypes(p,a1) ICOM_CALL1(get_MediaTypes,p,a1)
#define IPinInfo_Connect(p,a1) ICOM_CALL1(Connect,p,a1)
#define IPinInfo_ConnectDirect(p,a1) ICOM_CALL1(ConnectDirect,p,a1)
#define IPinInfo_ConnectWithType(p,a1,a2) ICOM_CALL2(ConnectWithType,p,a1,a2)
#define IPinInfo_Disconnect(p) ICOM_CALL (Disconnect,p)
#define IPinInfo_Render(p) ICOM_CALL (Render,p)

/**************************************************************************
 *
 * IQueueCommand interface
 *
 */

#define ICOM_INTERFACE IQueueCommand
#define IQueueCommand_METHODS \
    ICOM_METHOD9(HRESULT,InvokeAtStreamTime,IDeferredCommand**,a1,REFTIME,a2,GUID*,a3,long,a4,short,a5,long,a6,VARIANT*,a7,VARIANT*,a8,short*,a9) \
    ICOM_METHOD9(HRESULT,InvokeAtPresentationTime,IDeferredCommand**,a1,REFTIME,a2,GUID*,a3,long,a4,short,a5,long,a6,VARIANT*,a7,VARIANT*,a8,short*,a9)

#define IQueueCommand_IMETHODS \
    IUnknown_IMETHODS \
    IQueueCommand_METHODS

ICOM_DEFINE(IQueueCommand,IUnknown)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IQueueCommand_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IQueueCommand_AddRef(p) ICOM_CALL (AddRef,p)
#define IQueueCommand_Release(p) ICOM_CALL (Release,p)
    /*** IQueueCommand methods ***/
#define IQueueCommand_InvokeAtStreamTime(p,a1,a2,a3,a4,a5,a6,a7,a8,a9) ICOM_CALL9(InvokeAtStreamTime,p,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define IQueueCommand_InvokeAtPresentationTime(p,a1,a2,a3,a4,a5,a6,a7,a8,a9) ICOM_CALL9(InvokeAtPresentationTime,p,a1,a2,a3,a4,a5,a6,a7,a8,a9)

/**************************************************************************
 *
 * IRegFilterInfo interface
 *
 */

#define ICOM_INTERFACE IRegFilterInfo
#define IRegFilterInfo_METHODS \
    ICOM_METHOD1(HRESULT,get_Name,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,Filter,IDispatch**,a1)

#define IRegFilterInfo_IMETHODS \
    IDispatch_IMETHODS \
    IRegFilterInfo_METHODS

ICOM_DEFINE(IRegFilterInfo,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IRegFilterInfo_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IRegFilterInfo_AddRef(p) ICOM_CALL (AddRef,p)
#define IRegFilterInfo_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IRegFilterInfo_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IRegFilterInfo_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IRegFilterInfo_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IRegFilterInfo_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IRegFilterInfo methods ***/
#define IRegFilterInfo_get_Name(p,a1) ICOM_CALL1(get_Name,p,a1)
#define IRegFilterInfo_Filter(p,a1) ICOM_CALL1(Filter,p,a1)

/**************************************************************************
 *
 * IVideoWindow interface
 *
 */

#define ICOM_INTERFACE IVideoWindow
#define IVideoWindow_METHODS \
    ICOM_METHOD1(HRESULT,put_Caption,BSTR,a1) \
    ICOM_METHOD1(HRESULT,get_Caption,BSTR*,a1) \
    ICOM_METHOD1(HRESULT,put_WindowStyle,long,a1) \
    ICOM_METHOD1(HRESULT,get_WindowStyle,long*,a1) \
    ICOM_METHOD1(HRESULT,put_WindowStyleEx,long,a1) \
    ICOM_METHOD1(HRESULT,get_WindowStyleEx,long*,a1) \
    ICOM_METHOD1(HRESULT,put_AutoShow,long,a1) \
    ICOM_METHOD1(HRESULT,get_AutoShow,long*,a1) \
    ICOM_METHOD1(HRESULT,put_WindowState,long,a1) \
    ICOM_METHOD1(HRESULT,get_WindowState,long*,a1) \
    ICOM_METHOD1(HRESULT,put_BackgroundPalette,long,a1) \
    ICOM_METHOD1(HRESULT,get_BackgroundPalette,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Visible,long,a1) \
    ICOM_METHOD1(HRESULT,get_Visible,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Left,long,a1) \
    ICOM_METHOD1(HRESULT,get_Left,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Width,long,a1) \
    ICOM_METHOD1(HRESULT,get_Width,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Top,long,a1) \
    ICOM_METHOD1(HRESULT,get_Top,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Height,long,a1) \
    ICOM_METHOD1(HRESULT,get_Height,long*,a1) \
    ICOM_METHOD1(HRESULT,put_Owner,OAHWND,a1) \
    ICOM_METHOD1(HRESULT,get_Owner,OAHWND*,a1) \
    ICOM_METHOD1(HRESULT,put_MessageDrain,OAHWND,a1) \
    ICOM_METHOD1(HRESULT,get_MessageDrain,OAHWND*,a1) \
    ICOM_METHOD1(HRESULT,get_BorderColor,long*,a1) \
    ICOM_METHOD1(HRESULT,put_BorderColor,long,a1) \
    ICOM_METHOD1(HRESULT,get_FullScreenMode,long*,a1) \
    ICOM_METHOD1(HRESULT,put_FullScreenMode,long,a1) \
    ICOM_METHOD1(HRESULT,SetWindowForeground,long,a1) \
    ICOM_METHOD4(HRESULT,NotifyOwnerMessage,OAHWND,a1,long,a2,LONG_PTR,a3,LONG_PTR,a4) \
    ICOM_METHOD4(HRESULT,SetWindowPosition,long,a1,long,a2,long,a3,long,a4) \
    ICOM_METHOD4(HRESULT,GetWindowPosition,long*,a1,long*,a2,long*,a3,long*,a4) \
    ICOM_METHOD2(HRESULT,GetMinIdealImageSize,long*,a1,long*,a2) \
    ICOM_METHOD2(HRESULT,GetMaxIdealImageSize,long*,a1,long*,a2) \
    ICOM_METHOD4(HRESULT,GetRestorePosition,long*,a1,long*,a2,long*,a3,long*,a4) \
    ICOM_METHOD1(HRESULT,HideCursor,long,a1) \
    ICOM_METHOD1(HRESULT,IsCursorHidden,long*,a1)

#define IVideoWindow_IMETHODS \
    IDispatch_IMETHODS \
    IVideoWindow_METHODS

ICOM_DEFINE(IVideoWindow,IDispatch)
#undef ICOM_INTERFACE

    /*** IUnknown methods ***/
#define IVideoWindow_QueryInterface(p,a1,a2) ICOM_CALL2(QueryInterface,p,a1,a2)
#define IVideoWindow_AddRef(p) ICOM_CALL (AddRef,p)
#define IVideoWindow_Release(p) ICOM_CALL (Release,p)
    /*** IDispatch methods ***/
#define IVideoWindow_GetTypeInfoCount(p,a1) ICOM_CALL1(GetTypeInfoCount,p,a1)
#define IVideoWindow_GetTypeInfo(p,a1,a2,a3) ICOM_CALL3(GetTypeInfo,p,a1,a2,a3)
#define IVideoWindow_GetIDsOfNames(p,a1,a2,a3,a4,a5) ICOM_CALL5(GetIDsOfNames,p,a1,a2,a3,a4,a5)
#define IVideoWindow_Invoke(p,a1,a2,a3,a4,a5,a6,a7,a8) ICOM_CALL8(Invoke,p,a1,a2,a3,a4,a5,a6,a7,a8)
    /*** IVideoWindow methods ***/
#define IVideoWindow_put_Caption(p,a1) ICOM_CALL1(put_Caption,p,a1)
#define IVideoWindow_get_Caption(p,a1) ICOM_CALL1(get_Caption,p,a1)
#define IVideoWindow_put_WindowStyle(p,a1) ICOM_CALL1(put_WindowStyle,p,a1)
#define IVideoWindow_get_WindowStyle(p,a1) ICOM_CALL1(get_WindowStyle,p,a1)
#define IVideoWindow_put_WindowStyleEx(p,a1) ICOM_CALL1(put_WindowStyleEx,p,a1)
#define IVideoWindow_get_WindowStyleEx(p,a1) ICOM_CALL1(get_WindowStyleEx,p,a1)
#define IVideoWindow_put_AutoShow(p,a1) ICOM_CALL1(put_AutoShow,p,a1)
#define IVideoWindow_get_AutoShow(p,a1) ICOM_CALL1(get_AutoShow,p,a1)
#define IVideoWindow_put_WindowState(p,a1) ICOM_CALL1(put_WindowState,p,a1)
#define IVideoWindow_get_WindowState(p,a1) ICOM_CALL1(get_WindowState,p,a1)
#define IVideoWindow_put_BackgroundPalette(p,a1) ICOM_CALL1(put_BackgroundPalette,p,a1)
#define IVideoWindow_get_BackgroundPalette(p,a1) ICOM_CALL1(get_BackgroundPalette,p,a1)
#define IVideoWindow_put_Visible(p,a1) ICOM_CALL1(put_Visible,p,a1)
#define IVideoWindow_get_Visible(p,a1) ICOM_CALL1(get_Visible,p,a1)
#define IVideoWindow_put_Left(p,a1) ICOM_CALL1(put_Left,p,a1)
#define IVideoWindow_get_Left(p,a1) ICOM_CALL1(get_Left,p,a1)
#define IVideoWindow_put_Width(p,a1) ICOM_CALL1(put_Width,p,a1)
#define IVideoWindow_get_Width(p,a1) ICOM_CALL1(get_Width,p,a1)
#define IVideoWindow_put_Top(p,a1) ICOM_CALL1(put_Top,p,a1)
#define IVideoWindow_get_Top(p,a1) ICOM_CALL1(get_Top,p,a1)
#define IVideoWindow_put_Height(p,a1) ICOM_CALL1(put_Height,p,a1)
#define IVideoWindow_get_Height(p,a1) ICOM_CALL1(get_Height,p,a1)
#define IVideoWindow_put_Owner(p,a1) ICOM_CALL1(put_Owner,p,a1)
#define IVideoWindow_get_Owner(p,a1) ICOM_CALL1(get_Owner,p,a1)
#define IVideoWindow_put_MessageDrain(p,a1) ICOM_CALL1(put_MessageDrain,p,a1)
#define IVideoWindow_get_MessageDrain(p,a1) ICOM_CALL1(get_MessageDrain,p,a1)
#define IVideoWindow_get_BorderColor(p,a1) ICOM_CALL1(get_BorderColor,p,a1)
#define IVideoWindow_put_BorderColor(p,a1) ICOM_CALL1(put_BorderColor,p,a1)
#define IVideoWindow_get_FullScreenMode(p,a1) ICOM_CALL1(get_FullScreenMode,p,a1)
#define IVideoWindow_put_FullScreenMode(p,a1) ICOM_CALL1(put_FullScreenMode,p,a1)
#define IVideoWindow_SetWindowForeground(p,a1) ICOM_CALL1(SetWindowForeground,p,a1)
#define IVideoWindow_NotifyOwnerMessage(p,a1,a2,a3,a4) ICOM_CALL4(NotifyOwnerMessage,p,a1,a2,a3,a4)
#define IVideoWindow_SetWindowPosition(p,a1,a2,a3,a4) ICOM_CALL4(SetWindowPosition,p,a1,a2,a3,a4)
#define IVideoWindow_GetWindowPosition(p,a1,a2,a3,a4) ICOM_CALL4(GetWindowPosition,p,a1,a2,a3,a4)
#define IVideoWindow_GetMinIdealImageSize(p,a1,a2) ICOM_CALL2(GetMinIdealImageSize,p,a1,a2)
#define IVideoWindow_GetMaxIdealImageSize(p,a1,a2) ICOM_CALL2(GetMaxIdealImageSize,p,a1,a2)
#define IVideoWindow_GetRestorePosition(p,a1,a2,a3,a4) ICOM_CALL4(GetRestorePosition,p,a1,a2,a3,a4)
#define IVideoWindow_HideCursor(p,a1) ICOM_CALL1(HideCursor,p,a1)
#define IVideoWindow_IsCursorHidden(p,a1) ICOM_CALL1(IsCursorHidden,p,a1)

#endif  /* __WINE_CONTROL_H_ */
