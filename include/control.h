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

#ifndef __CONTROL_INCLUDED__
#define __CONTROL_INCLUDED__

#include "windef.h"
#include "wingdi.h"
#include "objbase.h"
#include "oleauto.h"

typedef struct IMediaControl IMediaControl;
typedef struct IBasicAudio IBasicAudio;
typedef struct IBasicVideo IBasicVideo;
typedef struct IVideoWindow IVideoWindow;
typedef struct IMediaEvent IMediaEvent;
typedef struct IMediaEventEx IMediaEventEx;

typedef long OAFilterState;
typedef LONG_PTR OAHWND;
typedef LONG_PTR OAEVENT;

#define INTERFACE IMediaControl
#define IMediaControl_METHODS \
    IDispatch_METHODS \
    STDMETHOD(Run)(THIS) PURE; \
    STDMETHOD(Pause)(THIS) PURE; \
    STDMETHOD(Stop)(THIS) PURE; \
    STDMETHOD(GetState)(THIS_  LONG   msTimeout, OAFilterState *  pfs) PURE; \
    STDMETHOD(RenderFile)(THIS_  BSTR   strFilename) PURE; \
    STDMETHOD(AddSourceFilter)(THIS_  BSTR   strFilename, IDispatch **  ppUnk) PURE; \
    STDMETHOD(get_FilterCollection)(THIS_  IDispatch **  ppUnk) PURE; \
    STDMETHOD(get_RegFilterCollection)(THIS_  IDispatch **  ppUnk) PURE; \
    STDMETHOD(StopWhenReady)(THIS) PURE;
ICOM_DEFINE(IMediaControl,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMediaControl_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IMediaControl_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IMediaControl_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IMediaControl_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IMediaControl_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IMediaControl_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IMediaControl_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IMediaControl methods ***/
#define IMediaControl_Run(p)   (p)->lpVtbl->Run(p)
#define IMediaControl_Pause(p)   (p)->lpVtbl->Pause(p)
#define IMediaControl_Stop(p)   (p)->lpVtbl->Stop(p)
#define IMediaControl_GetState(p,a,b)   (p)->lpVtbl->GetState(p,a,b)
#define IMediaControl_RenderFile(p,a)   (p)->lpVtbl->RenderFile(p,a)
#define IMediaControl_AddSourceFilter(p,a,b)   (p)->lpVtbl->AddSourceFilter(p,a,b)
#define IMediaControl_get_FilterCollection(p,a)   (p)->lpVtbl->get_FilterCollection(p,a)
#define IMediaControl_get_RegFilterCollection(p,a)   (p)->lpVtbl->get_RegFilterCollection(p,a)
#define IMediaControl_StopWhenReady(p)   (p)->lpVtbl->StopWhenReady(p)
#endif

#define INTERFACE IBasicAudio
#define IBasicAudio_METHODS \
    IDispatch_METHODS \
    STDMETHOD(put_Volume)(THIS_  long   lVolume) PURE; \
    STDMETHOD(get_Volume)(THIS_  long *  plVolume) PURE; \
    STDMETHOD(put_Balance)(THIS_  long   lBalance) PURE; \
    STDMETHOD(get_Balance)(THIS_  long *  plBalance) PURE;
ICOM_DEFINE(IBasicAudio,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBasicAudio_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IBasicAudio_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IBasicAudio_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IBasicAudio_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IBasicAudio_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IBasicAudio_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IBasicAudio_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IBasicAudio methods ***/
#define IBasicAudio_get_Volume(p,a)   (p)->lpVtbl->get_Volume(p,a)
#define IBasicAudio_put_Volume(p,a)   (p)->lpVtbl->put_Volume(p,a)
#define IBasicAudio_get_Balance(p,a)   (p)->lpVtbl->get_Balance(p,a)
#define IBasicAudio_put_Balance(p,a)   (p)->lpVtbl->put_Balance(p,a)
#endif

#define INTERFACE IBasicVideo
#define IBasicVideo_METHODS \
    IDispatch_METHODS \
    STDMETHOD(get_AvgTimePerFrame)(THIS_  REFTIME *  pAvgTimePerFrame) PURE; \
    STDMETHOD(get_BitRate)(THIS_  long *  pBitRate) PURE; \
    STDMETHOD(get_BitErrorRate)(THIS_  long *  pBitErrorRate) PURE; \
    STDMETHOD(get_VideoWidth)(THIS_  long *  pVideoWidth) PURE; \
    STDMETHOD(get_VideoHeight)(THIS_  long *  pVideoHeight) PURE; \
    STDMETHOD(put_SourceLeft)(THIS_  long   SourceLeft) PURE; \
    STDMETHOD(get_SourceLeft)(THIS_  long *  pSourceLeft) PURE; \
    STDMETHOD(put_SourceWidth)(THIS_  long   SourceWidth) PURE; \
    STDMETHOD(get_SourceWidth)(THIS_  long *  pSourceWidth) PURE; \
    STDMETHOD(put_SourceTop)(THIS_  long   SourceTop) PURE; \
    STDMETHOD(get_SourceTop)(THIS_  long *  pSourceTop) PURE; \
    STDMETHOD(put_SourceHeight)(THIS_  long   SourceHeight) PURE; \
    STDMETHOD(get_SourceHeight)(THIS_  long *  pSourceHeight) PURE; \
    STDMETHOD(put_DestinationLeft)(THIS_  long   DestinationLeft) PURE; \
    STDMETHOD(get_DestinationLeft)(THIS_  long *  pDestinationLeft) PURE; \
    STDMETHOD(put_DestinationWidth)(THIS_  long   DestinationWidth) PURE; \
    STDMETHOD(get_DestinationWidth)(THIS_  long *  pDestinationWidth) PURE; \
    STDMETHOD(put_DestinationTop)(THIS_  long   DestinationTop) PURE; \
    STDMETHOD(get_DestinationTop)(THIS_  long *  pDestinationTop) PURE; \
    STDMETHOD(put_DestinationHeight)(THIS_  long   DestinationHeight) PURE; \
    STDMETHOD(get_DestinationHeight)(THIS_  long *  pDestinationHeight) PURE; \
    STDMETHOD(SetSourcePosition)(THIS_  long   Left, long   Top, long   Width, long   Height) PURE; \
    STDMETHOD(GetSourcePosition)(THIS_  long *  pLeft, long *  pTop, long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(SetDefaultSourcePosition)(THIS) PURE; \
    STDMETHOD(SetDestinationPosition)(THIS_  long   Left, long   Top, long   Width, long   Height) PURE; \
    STDMETHOD(GetDestinationPosition)(THIS_  long *  pLeft, long *  pTop, long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(SetDefaultDestinationPosition)(THIS) PURE; \
    STDMETHOD(GetVideoSize)(THIS_  long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(GetVideoPaletteEntries)(THIS_  long   StartIndex, long   Entries, long *  pRetrieved, long *  pPalette) PURE; \
    STDMETHOD(GetCurrentImage)(THIS_  long *  pBufferSize, long *  pDIBImage) PURE; \
    STDMETHOD(IsUsingDefaultSource)(THIS) PURE; \
    STDMETHOD(IsUsingDefaultDestination)(THIS) PURE;
ICOM_DEFINE(IBasicVideo,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IBasicVideo_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IBasicVideo_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IBasicVideo_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IBasicVideo_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IBasicVideo_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IBasicVideo_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IBasicVideo_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IBasicVideo methods ***/
#define IBasicVideo_get_AvgTimePerFrame(p,a)   (p)->lpVtbl->get_AvgTimePerFrame(p,a)
#define IBasicVideo_get_BitRate(p,a)   (p)->lpVtbl->get_BitRate(p,a)
#define IBasicVideo_get_BitErrorRate(p,a)   (p)->lpVtbl->get_BitErrorRate(p,a)
#define IBasicVideo_get_VideoWidth(p,a)   (p)->lpVtbl->get_VideoWidth(p,a)
#define IBasicVideo_get_VideoHeight(p,a)   (p)->lpVtbl->get_VideoHeight(p,a)
#define IBasicVideo_put_SourceLeft(p,a)   (p)->lpVtbl->put_SourceLeft(p,a)
#define IBasicVideo_get_SourceLeft(p,a)   (p)->lpVtbl->get_SourceLeft(p,a)
#define IBasicVideo_put_SourceWidth(p,a)   (p)->lpVtbl->put_SourceWidth(p,a)
#define IBasicVideo_get_SourceWidth(p,a)   (p)->lpVtbl->get_SourceWidth(p,a)
#define IBasicVideo_put_SourceTop(p,a)   (p)->lpVtbl->put_SourceTop(p,a)
#define IBasicVideo_get_SourceTop(p,a)   (p)->lpVtbl->get_SourceTop(p,a)
#define IBasicVideo_put_SourceHeight(p,a)   (p)->lpVtbl->put_SourceHeight(p,a)
#define IBasicVideo_get_SourceHeight(p,a)   (p)->lpVtbl->get_SourceHeight(p,a)
#define IBasicVideo_put_DestinationLeft(p,a)   (p)->lpVtbl->put_DestinationLeft(p,a)
#define IBasicVideo_get_DestinationLeft(p,a)   (p)->lpVtbl->get_DestinationLeft(p,a)
#define IBasicVideo_put_DestinationWidth(p,a)   (p)->lpVtbl->put_DestinationWidth(p,a)
#define IBasicVideo_get_DestinationWidth(p,a)   (p)->lpVtbl->get_DestinationWidth(p,a)
#define IBasicVideo_put_DestinationTop(p,a)   (p)->lpVtbl->put_DestinationTop(p,a)
#define IBasicVideo_get_DestinationTop(p,a)   (p)->lpVtbl->get_DestinationTop(p,a)
#define IBasicVideo_put_DestinationHeight(p,a)   (p)->lpVtbl->put_DestinationHeight(p,a)
#define IBasicVideo_get_DestinationHeight(p,a)   (p)->lpVtbl->get_DestinationHeight(p,a)
#define IBasicVideo_SetSourcePosition(p,a,b,c,d)   (p)->lpVtbl->SetSourcePosition(p,a,b,c,d)
#define IBasicVideo_GetSourcePosition(p,a,b,c,d)   (p)->lpVtbl->GetSourcePosition(p,a,b,c,d)
#define IBasicVideo_SetDefaultSourcePosition(p)   (p)->lpVtbl->SetDefaultSourcePosition(p)
#define IBasicVideo_SetDestinationPosition(p,a,b,c,d)   (p)->lpVtbl->SetDestinationPosition(p,a,b,c,d)
#define IBasicVideo_GetDestinationPosition(p,a,b,c,d)   (p)->lpVtbl->GetDestinationPosition(p,a,b,c,d)
#define IBasicVideo_SetDefaultDestinationPosition(p)   (p)->lpVtbl->SetDefaultDestinationPosition(p)
#define IBasicVideo_GetVideoSize(p,a,b)   (p)->lpVtbl->GetVideoSize(p,a,b)
#define IBasicVideo_GetVideoPaletteEntries(p,a,b,c,d)   (p)->lpVtbl->GetVideoPaletteEntries(p,a,b,c,d)
#define IBasicVideo_GetCurrentImage(p,a,b)   (p)->lpVtbl->GetCurrentImage(p,a,b)
#define IBasicVideo_IsUsingDefaultSource(p)   (p)->lpVtbl->IsUsingDefaultSource(p)
#define IBasicVideo_IsUsingDefaultDestination(p)   (p)->lpVtbl->IsUsingDefaultDestination(p)
#endif

#define INTERFACE IVideoWindow
#define IVideoWindow_METHODS \
    IDispatch_METHODS \
    STDMETHOD(put_Caption)(THIS_  BSTR   strCaption) PURE; \
    STDMETHOD(get_Caption)(THIS_  BSTR *  strCaption) PURE; \
    STDMETHOD(put_WindowStyle)(THIS_  long   WindowStyle) PURE; \
    STDMETHOD(get_WindowStyle)(THIS_  long *  WindowStyle) PURE; \
    STDMETHOD(put_WindowStyleEx)(THIS_  long   WindowStyleEx) PURE; \
    STDMETHOD(get_WindowStyleEx)(THIS_  long *  WindowStyleEx) PURE; \
    STDMETHOD(put_AutoShow)(THIS_  long   AutoShow) PURE; \
    STDMETHOD(get_AutoShow)(THIS_  long *  AutoShow) PURE; \
    STDMETHOD(put_WindowState)(THIS_  long   WindowState) PURE; \
    STDMETHOD(get_WindowState)(THIS_  long *  WindowState) PURE; \
    STDMETHOD(put_BackgroundPalette)(THIS_  long   BackgroundPalette) PURE; \
    STDMETHOD(get_BackgroundPalette)(THIS_  long *  pBackgroundPalette) PURE; \
    STDMETHOD(put_Visible)(THIS_  long   Visible) PURE; \
    STDMETHOD(get_Visible)(THIS_  long *  pVisible) PURE; \
    STDMETHOD(put_Left)(THIS_  long   Left) PURE; \
    STDMETHOD(get_Left)(THIS_  long *  pLeft) PURE; \
    STDMETHOD(put_Width)(THIS_  long   Width) PURE; \
    STDMETHOD(get_Width)(THIS_  long *  pWidth) PURE; \
    STDMETHOD(put_Top)(THIS_  long   Top) PURE; \
    STDMETHOD(get_Top)(THIS_  long *  pTop) PURE; \
    STDMETHOD(put_Height)(THIS_  long   Height) PURE; \
    STDMETHOD(get_Height)(THIS_  long *  pHeight) PURE; \
    STDMETHOD(put_Owner)(THIS_  OAHWND   Owner) PURE; \
    STDMETHOD(get_Owner)(THIS_  OAHWND *  Owner) PURE; \
    STDMETHOD(put_MessageDrain)(THIS_  OAHWND   Drain) PURE; \
    STDMETHOD(get_MessageDrain)(THIS_  OAHWND *  Drain) PURE; \
    STDMETHOD(get_BorderColor)(THIS_  long *  Color) PURE; \
    STDMETHOD(put_BorderColor)(THIS_  long   Color) PURE; \
    STDMETHOD(get_FullScreenMode)(THIS_  long *  FullScreenMode) PURE; \
    STDMETHOD(put_FullScreenMode)(THIS_  long   FullScreenMode) PURE; \
    STDMETHOD(SetWindowForeground)(THIS_  long   Focus) PURE; \
    STDMETHOD(NotifyOwnerMessage)(THIS_  OAHWND   hwnd, long   uMsg, LONG_PTR   wParam, LONG_PTR   lParam) PURE; \
    STDMETHOD(SetWindowPosition)(THIS_  long   Left, long   Top, long   Width, long   Height) PURE; \
    STDMETHOD(GetWindowPosition)(THIS_  long *  pLeft, long *  pTop, long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(GetMinIdealImageSize)(THIS_  long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(GetMaxIdealImageSize)(THIS_  long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(GetRestorePosition)(THIS_  long *  pLeft, long *  pTop, long *  pWidth, long *  pHeight) PURE; \
    STDMETHOD(HideCursor)(THIS_  long   HideCursor) PURE; \
    STDMETHOD(IsCursorHidden)(THIS_  long *  CursorHidden) PURE;
ICOM_DEFINE(IVideoWindow,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IVideoWindow_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IVideoWindow_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IVideoWindow_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IVideoWindow_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IVideoWindow_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IVideoWindow_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IVideoWindow_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IVideoWindow methods ***/
#define IVideoWindow_put_Caption(p,a)   (p)->lpVtbl->put_Caption(p,a)
#define IVideoWindow_get_Caption(p,a)   (p)->lpVtbl->get_Caption(p,a)
#define IVideoWindow_put_WindowStyle(p,a)   (p)->lpVtbl->put_WindowStyle(p,a)
#define IVideoWindow_get_WindowStyle(p,a)   (p)->lpVtbl->get_WindowStyle(p,a)
#define IVideoWindow_put_WindowStyleEx(p,a)   (p)->lpVtbl->put_WindowStyleEx(p,a)
#define IVideoWindow_get_WindowStyleEx(p,a)   (p)->lpVtbl->get_WindowStyleEx(p,a)
#define IVideoWindow_put_AutoShow(p,a)   (p)->lpVtbl->put_AutoShow(p,a)
#define IVideoWindow_get_AutoShow(p,a)   (p)->lpVtbl->get_AutoShow(p,a)
#define IVideoWindow_put_WindowState(p,a)   (p)->lpVtbl->put_WindowState(p,a)
#define IVideoWindow_get_WindowState(p,a)   (p)->lpVtbl->get_WindowState(p,a)
#define IVideoWindow_put_BackgroundPalette(p,a)   (p)->lpVtbl->put_BackgroundPalette(p,a)
#define IVideoWindow_get_BackgroundPalette(p,a)   (p)->lpVtbl->get_BackgroundPalette(p,a)
#define IVideoWindow_put_Visible(p,a)   (p)->lpVtbl->put_Visible(p,a)
#define IVideoWindow_get_Visible(p,a)   (p)->lpVtbl->get_Visible(p,a)
#define IVideoWindow_put_Left(p,a)   (p)->lpVtbl->put_Left(p,a)
#define IVideoWindow_get_Left(p,a)   (p)->lpVtbl->get_Left(p,a)
#define IVideoWindow_put_Width(p,a)   (p)->lpVtbl->put_Width(p,a)
#define IVideoWindow_get_Width(p,a)   (p)->lpVtbl->get_Width(p,a)
#define IVideoWindow_put_Top(p,a)   (p)->lpVtbl->put_Top(p,a)
#define IVideoWindow_get_Top(p,a)   (p)->lpVtbl->get_Top(p,a)
#define IVideoWindow_put_Height(p,a)   (p)->lpVtbl->put_Height(p,a)
#define IVideoWindow_get_Height(p,a)   (p)->lpVtbl->get_Height(p,a)
#define IVideoWindow_put_Owner(p,a)   (p)->lpVtbl->put_Owner(p,a)
#define IVideoWindow_get_Owner(p,a)   (p)->lpVtbl->get_Owner(p,a)
#define IVideoWindow_put_MessageDrain(p,a)   (p)->lpVtbl->put_MessageDrain(p,a)
#define IVideoWindow_get_MessageDrain(p,a)   (p)->lpVtbl->get_MessageDrain(p,a)
#define IVideoWindow_get_BorderColor(p,a)   (p)->lpVtbl->get_BorderColor(p,a)
#define IVideoWindow_put_BorderColor(p,a)   (p)->lpVtbl->put_BorderColor(p,a)
#define IVideoWindow_get_FullScreenMode(p,a)   (p)->lpVtbl->get_FullScreenMode(p,a)
#define IVideoWindow_put_FullScreenMode(p,a)   (p)->lpVtbl->put_FullScreenMode(p,a)
#define IVideoWindow_SetWindowForeground(p,a)   (p)->lpVtbl->SetWindowForeground(p,a)
#define IVideoWindow_NotifyOwnerMessage(p,a,b,c,d)   (p)->lpVtbl->NotifyOwnerMessage(p,a,b,c,d)
#define IVideoWindow_SetWindowPosition(p,a,b,c,d)   (p)->lpVtbl->SetWindowPosition(p,a,b,c,d)
#define IVideoWindow_GetWindowPosition(p,a,b,c,d)   (p)->lpVtbl->GetWindowPosition(p,a,b,c,d)
#define IVideoWindow_GetMinIdealImageSize(p,a,b)   (p)->lpVtbl->GetMinIdealImageSize(p,a,b)
#define IVideoWindow_GetMaxIdealImageSize(p,a,b)   (p)->lpVtbl->GetMaxIdealImageSize(p,a,b)
#define IVideoWindow_GetRestorePosition(p,a,b,c,d)   (p)->lpVtbl->GetRestorePosition(p,a,b,c,d)
#define IVideoWindow_HideCursor(p,a)   (p)->lpVtbl->HideCursor(p,a)
#define IVideoWindow_IsCursorHidden(p,a)   (p)->lpVtbl->IsCursorHidden(p,a)
#endif

#define INTERFACE IMediaEvent
#define IMediaEvent_METHODS \
    IDispatch_METHODS \
    STDMETHOD(GetEventHandle)(THIS_  OAEVENT *  hEvent) PURE; \
    STDMETHOD(GetEvent)(THIS_  long *  lEventCode, LONG_PTR *  lParam1, LONG_PTR *  lParam2, long   msTimeout) PURE; \
    STDMETHOD(WaitForCompletion)(THIS_  long   msTimeout, long *  pEvCode) PURE; \
    STDMETHOD(CancelDefaultHandling)(THIS_  long   lEvCode) PURE; \
    STDMETHOD(RestoreDefaultHandling)(THIS_  long   lEvCode) PURE; \
    STDMETHOD(FreeEventParams)(THIS_  long   lEvCode, LONG_PTR   lParam1, LONG_PTR   lParam2) PURE;
ICOM_DEFINE(IMediaEvent,IDispatch)
#undef INTERFACE
     
#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMediaEvent_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IMediaEvent_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IMediaEvent_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IMediaEvent_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IMediaEvent_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IMediaEvent_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IMediaEvent_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IMediaEvent methods ***/
#define IMediaEvent_GetEventHandle(p,a)   (p)->lpVtbl->GetEventHandle(p,a)
#define IMediaEvent_GetEvent(p,a,b,c,d)   (p)->lpVtbl->GetEvent(p,a,b,c,d)
#define IMediaEvent_WaitForCompletion(p,a,b)   (p)->lpVtbl->WaitForCompletion(p,a,b)
#define IMediaEvent_CancelDefaultHandling(p,a)   (p)->lpVtbl->CancelDefaultHandling(p,a)
#define IMediaEvent_RestoreDefaultHandling(p,a)   (p)->lpVtbl->RestoreDefaultHandling(p,a)
#define IMediaEvent_FreeEventParams(p,a,b,c)   (p)->lpVtbl->FreeEventParams(p,a,b,c)
#endif

#define INTERFACE IMediaEventEx
#define IMediaEventEx_METHODS \
    IMediaEvent_METHODS \
    STDMETHOD(SetNotifyWindow)(THIS_  OAHWND   hwnd, long   lMsg, LONG_PTR   lInstanceData) PURE; \
    STDMETHOD(SetNotifyFlags)(THIS_  long   lNoNotifyFlags) PURE; \
    STDMETHOD(GetNotifyFlags)(THIS_  long *  lplNoNotifyFlags) PURE;
ICOM_DEFINE(IMediaEventEx,IMediaEvent)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMediaEventEx_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IMediaEventEx_AddRef(p)   (p)->lpVtbl->AddRef(p)
#define IMediaEventEx_Release(p)   (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IMediaEventEx_GetTypeInfoCount(p,a)   (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IMediaEventEx_GetTypeInfo(p,a,b,c)   (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IMediaEventEx_GetIDsOfNames(p,a,b,c,d,e)   (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IMediaEventEx_Invoke(p,a,b,c,d,e,f,g,h)   (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IMediaEvent methods ***/
#define IMediaEventEx_GetEventHandle(p,a)   (p)->lpVtbl->GetEventHandle(p,a)
#define IMediaEventEx_GetEvent(p,a,b,c,d)   (p)->lpVtbl->GetEvent(p,a,b,c,d)
#define IMediaEventEx_WaitForCompletion(p,a,b)   (p)->lpVtbl->WaitForCompletion(p,a,b)
#define IMediaEventEx_CancelDefaultHandling(p,a)   (p)->lpVtbl->CancelDefaultHandling(p,a)
#define IMediaEventEx_RestoreDefaultHandling(p,a)   (p)->lpVtbl->RestoreDefaultHandling(p,a)
#define IMediaEventEx_FreeEventParams(p,a,b,c)   (p)->lpVtbl->FreeEventParams(p,a,b,c)
/*** IMediaEventEx methods ***/
#define IMediaEventEx_SetNotifyWindow(p,a,b,c)   (p)->lpVtbl->SetNotifyWindow(p,a,b,c)
#define IMediaEventEx_SetNotifyFlags(p,a)   (p)->lpVtbl->SetNotifyFlags(p,a)
#define IMediaEventEx_GetNotifyFlags(p,a)   (p)->lpVtbl->GetNotifyFlags(p,a)
#endif

#endif /* __CONTROL_INCLUDED__ */
