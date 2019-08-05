/*
 * Video Renderer (Fullscreen and Windowed using Direct Draw)
 *
 * Copyright 2004 Christian Costa
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

#include "quartz_private.h"
#include "pin.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"
#include "dvdmedia.h"

#include <assert.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct VideoRendererImpl
{
    BaseRenderer renderer;
    BaseControlWindow baseControlWindow;
    BaseControlVideo baseControlVideo;

    BOOL init;
    HANDLE hThread;

    DWORD ThreadID;
    HANDLE hEvent;
    BOOL ThreadResult;
    RECT SourceRect;
    RECT DestRect;
    RECT WindowPos;
    LONG VideoWidth;
    LONG VideoHeight;
    LONG FullScreenMode;
} VideoRendererImpl;

static inline VideoRendererImpl *impl_from_BaseWindow(BaseWindow *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, baseControlWindow.baseWindow);
}

static inline VideoRendererImpl *impl_from_BaseRenderer(BaseRenderer *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, renderer);
}

static inline VideoRendererImpl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, renderer.filter.IBaseFilter_iface);
}

static inline VideoRendererImpl *impl_from_IVideoWindow(IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, baseControlWindow.IVideoWindow_iface);
}

static inline VideoRendererImpl *impl_from_BaseControlVideo(BaseControlVideo *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, baseControlVideo);
}

static DWORD WINAPI MessageLoop(LPVOID lpParameter)
{
    VideoRendererImpl* This = lpParameter;
    MSG msg; 
    BOOL fGotMessage;

    TRACE("Starting message loop\n");

    if (FAILED(BaseWindowImpl_PrepareWindow(&This->baseControlWindow.baseWindow)))
    {
        This->ThreadResult = FALSE;
        SetEvent(This->hEvent);
        return 0;
    }

    This->ThreadResult = TRUE;
    SetEvent(This->hEvent);

    while ((fGotMessage = GetMessageW(&msg, NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg); 
        DispatchMessageW(&msg);
    }

    TRACE("End of message loop\n");

    return msg.wParam;
}

static BOOL CreateRenderingSubsystem(VideoRendererImpl* This)
{
    This->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!This->hEvent)
        return FALSE;

    This->hThread = CreateThread(NULL, 0, MessageLoop, This, 0, &This->ThreadID);
    if (!This->hThread)
    {
        CloseHandle(This->hEvent);
        return FALSE;
    }

    WaitForSingleObject(This->hEvent, INFINITE);

    if (!This->ThreadResult)
    {
        CloseHandle(This->hEvent);
        CloseHandle(This->hThread);
        return FALSE;
    }

    return TRUE;
}

static void VideoRenderer_AutoShowWindow(VideoRendererImpl *This)
{
    if (!This->init && (!This->WindowPos.right || !This->WindowPos.top))
    {
        DWORD style = GetWindowLongW(This->baseControlWindow.baseWindow.hWnd, GWL_STYLE);
        DWORD style_ex = GetWindowLongW(This->baseControlWindow.baseWindow.hWnd, GWL_EXSTYLE);

        if (!This->WindowPos.right)
        {
            if (This->DestRect.right)
            {
                This->WindowPos.left = This->DestRect.left;
                This->WindowPos.right = This->DestRect.right;
            }
            else
            {
                This->WindowPos.left = This->SourceRect.left;
                This->WindowPos.right = This->SourceRect.right;
            }
        }
        if (!This->WindowPos.bottom)
        {
            if (This->DestRect.bottom)
            {
                This->WindowPos.top = This->DestRect.top;
                This->WindowPos.bottom = This->DestRect.bottom;
            }
            else
            {
                This->WindowPos.top = This->SourceRect.top;
                This->WindowPos.bottom = This->SourceRect.bottom;
            }
        }

        AdjustWindowRectEx(&This->WindowPos, style, FALSE, style_ex);

        TRACE("WindowPos: %s\n", wine_dbgstr_rect(&This->WindowPos));
        SetWindowPos(This->baseControlWindow.baseWindow.hWnd, NULL,
            This->WindowPos.left,
            This->WindowPos.top,
            This->WindowPos.right - This->WindowPos.left,
            This->WindowPos.bottom - This->WindowPos.top,
            SWP_NOZORDER|SWP_NOMOVE|SWP_DEFERERASE);

        GetClientRect(This->baseControlWindow.baseWindow.hWnd, &This->DestRect);
    }
    else if (!This->init)
        This->DestRect = This->WindowPos;
    This->init = TRUE;
    if (This->baseControlWindow.AutoShow)
        ShowWindow(This->baseControlWindow.baseWindow.hWnd, SW_SHOW);
}

static DWORD VideoRenderer_SendSampleData(VideoRendererImpl* This, LPBYTE data, DWORD size)
{
    AM_MEDIA_TYPE amt;
    HRESULT hr = S_OK;
    BITMAPINFOHEADER *bmiHeader;

    TRACE("(%p)->(%p, %d)\n", This, data, size);

    hr = IPin_ConnectionMediaType(&This->renderer.sink.pin.IPin_iface, &amt);
    if (FAILED(hr)) {
        ERR("Unable to retrieve media type\n");
        return hr;
    }

    if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)amt.pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)amt.pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&amt.subtype));
        return VFW_E_RUNTIME_ERROR;
    }

    TRACE("biSize = %d\n", bmiHeader->biSize);
    TRACE("biWidth = %d\n", bmiHeader->biWidth);
    TRACE("biHeight = %d\n", bmiHeader->biHeight);
    TRACE("biPlanes = %d\n", bmiHeader->biPlanes);
    TRACE("biBitCount = %d\n", bmiHeader->biBitCount);
    TRACE("biCompression = %s\n", debugstr_an((LPSTR)&(bmiHeader->biCompression), 4));
    TRACE("biSizeImage = %d\n", bmiHeader->biSizeImage);

    if (!This->baseControlWindow.baseWindow.hDC) {
        ERR("Cannot get DC from window!\n");
        return E_FAIL;
    }

    TRACE("Src Rect: %s\n", wine_dbgstr_rect(&This->SourceRect));
    TRACE("Dst Rect: %s\n", wine_dbgstr_rect(&This->DestRect));

    StretchDIBits(This->baseControlWindow.baseWindow.hDC, This->DestRect.left, This->DestRect.top, This->DestRect.right -This->DestRect.left,
                  This->DestRect.bottom - This->DestRect.top, This->SourceRect.left, This->SourceRect.top,
                  This->SourceRect.right - This->SourceRect.left, This->SourceRect.bottom - This->SourceRect.top,
                  data, (BITMAPINFO *)bmiHeader, DIB_RGB_COLORS, SRCCOPY);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_ShouldDrawSampleNow(BaseRenderer *This, IMediaSample *pSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime)
{
    /* Preroll means the sample isn't shown, this is used for key frames and things like that */
    if (IMediaSample_IsPreroll(pSample) == S_OK)
        return E_FAIL;
    return S_FALSE;
}

static HRESULT WINAPI VideoRenderer_DoRenderSample(BaseRenderer* iface, IMediaSample * pSample)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);
    LPBYTE pbSrcStream = NULL;
    LONG cbSrcStream = 0;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, pSample);

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("val %p %d\n", pbSrcStream, cbSrcStream);

#if 0 /* For debugging purpose */
    {
        int i;
        for(i = 0; i < cbSrcStream; i++)
        {
            if ((i!=0) && !(i%16))
                TRACE("\n");
                TRACE("%02x ", pbSrcStream[i]);
        }
        TRACE("\n");
    }
#endif

    SetEvent(This->hEvent);
    if (This->renderer.filter.state == State_Paused)
    {
        VideoRenderer_SendSampleData(This, pbSrcStream, cbSrcStream);
        SetEvent(This->hEvent);
        if (This->renderer.filter.state == State_Paused)
        {
            /* Flushing */
            return S_OK;
        }
        if (This->renderer.filter.state == State_Stopped)
        {
            return VFW_E_WRONG_STATE;
        }
    } else {
        VideoRenderer_SendSampleData(This, pbSrcStream, cbSrcStream);
    }
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB32) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB24) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB565) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB8))
    {
        LONG height;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
        {
            VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pmt->pbFormat;
            This->SourceRect.left = 0;
            This->SourceRect.top = 0;
            This->SourceRect.right = This->VideoWidth = format->bmiHeader.biWidth;
            height = format->bmiHeader.biHeight;
            if (height < 0)
                This->SourceRect.bottom = This->VideoHeight = -height;
            else
                This->SourceRect.bottom = This->VideoHeight = height;
        }
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
        {
            VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)pmt->pbFormat;

            This->SourceRect.left = 0;
            This->SourceRect.top = 0;
            This->SourceRect.right = This->VideoWidth = format2->bmiHeader.biWidth;
            height = format2->bmiHeader.biHeight;
            if (height < 0)
                This->SourceRect.bottom = This->VideoHeight = -height;
            else
                This->SourceRect.bottom = This->VideoHeight = height;
        }
        else
        {
            WARN("Format type %s not supported\n", debugstr_guid(&pmt->formattype));
            return S_FALSE;
        }
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI VideoRenderer_EndFlush(BaseRenderer* iface)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)->()\n", iface);

    if (This->renderer.pMediaSample) {
        ResetEvent(This->hEvent);
        LeaveCriticalSection(&iface->filter.csFilter);
        LeaveCriticalSection(&iface->csRenderLock);
        WaitForSingleObject(This->hEvent, INFINITE);
        EnterCriticalSection(&iface->csRenderLock);
        EnterCriticalSection(&iface->filter.csFilter);
    }
    if (This->renderer.filter.state == State_Paused) {
        ResetEvent(This->hEvent);
    }

    return BaseRendererImpl_EndFlush(iface);
}

static void video_renderer_destroy(BaseRenderer *iface)
{
    VideoRendererImpl *filter = impl_from_BaseRenderer(iface);

    BaseControlWindow_Destroy(&filter->baseControlWindow);
    BaseControlVideo_Destroy(&filter->baseControlVideo);
    PostThreadMessageW(filter->ThreadID, WM_QUIT, 0, 0);
    WaitForSingleObject(filter->hThread, INFINITE);
    CloseHandle(filter->hThread);
    CloseHandle(filter->hEvent);

    strmbase_renderer_cleanup(&filter->renderer);
    CoTaskMemFree(filter);
}

static HRESULT video_renderer_query_interface(BaseRenderer *iface, REFIID iid, void **out)
{
    VideoRendererImpl *filter = impl_from_BaseRenderer(iface);

    if (IsEqualGUID(iid, &IID_IBasicVideo))
        *out = &filter->baseControlVideo.IBasicVideo_iface;
    else if (IsEqualGUID(iid, &IID_IVideoWindow))
        *out = &filter->baseControlWindow.IVideoWindow_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static void video_renderer_stop_stream(BaseRenderer *iface)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)->()\n", This);

    SetEvent(This->hEvent);
    if (This->baseControlWindow.AutoShow)
        /* Black it out */
        RedrawWindow(This->baseControlWindow.baseWindow.hWnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

static void video_renderer_start_stream(BaseRenderer *iface)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)\n", This);

    if (This->renderer.sink.pin.pConnectedTo
        && (This->renderer.filter.state == State_Stopped || !This->renderer.sink.end_of_stream))
    {
        if (This->renderer.filter.state == State_Stopped)
        {
            ResetEvent(This->hEvent);
            VideoRenderer_AutoShowWindow(This);
        }
    }
}

static LPWSTR WINAPI VideoRenderer_GetClassWindowStyles(BaseWindow *This, DWORD *pClassStyles, DWORD *pWindowStyles, DWORD *pWindowStylesEx)
{
    static const WCHAR classnameW[] = { 'W','i','n','e',' ','A','c','t','i','v','e','M','o','v','i','e',' ','C','l','a','s','s',0 };

    *pClassStyles = 0;
    *pWindowStyles = WS_SIZEBOX;
    *pWindowStylesEx = 0;

    return (LPWSTR)classnameW;
}

static RECT WINAPI VideoRenderer_GetDefaultRect(BaseWindow *iface)
{
    VideoRendererImpl *This = impl_from_BaseWindow(iface);
    static RECT defRect;

    SetRect(&defRect, 0, 0, This->VideoWidth, This->VideoHeight);

    return defRect;
}

static BOOL WINAPI VideoRenderer_OnSize(BaseWindow *iface, LONG Width, LONG Height)
{
    VideoRendererImpl *This = impl_from_BaseWindow(iface);

    TRACE("WM_SIZE %d %d\n", Width, Height);
    GetClientRect(iface->hWnd, &This->DestRect);
    TRACE("WM_SIZING: DestRect=(%d,%d),(%d,%d)\n",
        This->DestRect.left,
        This->DestRect.top,
        This->DestRect.right - This->DestRect.left,
        This->DestRect.bottom - This->DestRect.top);
    return BaseWindowImpl_OnSize(iface, Width, Height);
}

static const BaseRendererFuncTable BaseFuncTable =
{
    .pfnCheckMediaType = VideoRenderer_CheckMediaType,
    .pfnDoRenderSample = VideoRenderer_DoRenderSample,
    .renderer_start_stream = video_renderer_start_stream,
    .renderer_stop_stream = video_renderer_stop_stream,
    .pfnShouldDrawSampleNow = VideoRenderer_ShouldDrawSampleNow,
    .pfnEndFlush = VideoRenderer_EndFlush,
    .renderer_destroy = video_renderer_destroy,
    .renderer_query_interface = video_renderer_query_interface,
};

static const BaseWindowFuncTable renderer_BaseWindowFuncTable = {
    VideoRenderer_GetClassWindowStyles,
    VideoRenderer_GetDefaultRect,
    NULL,
    BaseControlWindowImpl_PossiblyEatMessage,
    VideoRenderer_OnSize
};

static HRESULT WINAPI VideoRenderer_GetSourceRect(BaseControlVideo* iface, RECT *pSourceRect)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    CopyRect(pSourceRect,&This->SourceRect);
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetStaticImage(BaseControlVideo* iface, LONG *pBufferSize, LONG *pDIBImage)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    AM_MEDIA_TYPE *amt = &This->renderer.sink.pin.mtCurrent;
    BITMAPINFOHEADER *bmiHeader;
    LONG needed_size;
    char *ptr;

    FIXME("(%p/%p)->(%p, %p): partial stub\n", This, iface, pBufferSize, pDIBImage);

    EnterCriticalSection(&This->renderer.filter.csFilter);

    if (!This->renderer.pMediaSample)
    {
         LeaveCriticalSection(&This->renderer.filter.csFilter);
         return (This->renderer.filter.state == State_Paused ? E_UNEXPECTED : VFW_E_NOT_PAUSED);
    }

    if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)amt->pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&amt->formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)amt->pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&amt->subtype));
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return VFW_E_RUNTIME_ERROR;
    }

    needed_size = bmiHeader->biSize;
    needed_size += IMediaSample_GetActualDataLength(This->renderer.pMediaSample);

    if (!pDIBImage)
    {
        *pBufferSize = needed_size;
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return S_OK;
    }

    if (needed_size < *pBufferSize)
    {
        ERR("Buffer too small %u/%u\n", needed_size, *pBufferSize);
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        return E_FAIL;
    }
    *pBufferSize = needed_size;

    memcpy(pDIBImage, bmiHeader, bmiHeader->biSize);
    IMediaSample_GetPointer(This->renderer.pMediaSample, (BYTE **)&ptr);
    memcpy((char *)pDIBImage + bmiHeader->biSize, ptr, IMediaSample_GetActualDataLength(This->renderer.pMediaSample));

    LeaveCriticalSection(&This->renderer.filter.csFilter);
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetTargetRect(BaseControlVideo* iface, RECT *pTargetRect)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    CopyRect(pTargetRect,&This->DestRect);
    return S_OK;
}

static VIDEOINFOHEADER* WINAPI VideoRenderer_GetVideoFormat(BaseControlVideo* iface)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    AM_MEDIA_TYPE *pmt;

    TRACE("(%p/%p)\n", This, iface);

    pmt = &This->renderer.sink.pin.mtCurrent;
    if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        return (VIDEOINFOHEADER*)pmt->pbFormat;
    } else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2)) {
        static VIDEOINFOHEADER vih;
        VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
        memcpy(&vih,vih2,sizeof(VIDEOINFOHEADER));
        memcpy(&vih.bmiHeader, &vih2->bmiHeader, sizeof(BITMAPINFOHEADER));
        return &vih;
    } else {
        ERR("Unknown format type %s\n", qzdebugstr_guid(&pmt->formattype));
        return NULL;
    }
}

static HRESULT WINAPI VideoRenderer_IsDefaultSourceRect(BaseControlVideo* iface)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_IsDefaultTargetRect(BaseControlVideo* iface)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_SetDefaultSourceRect(BaseControlVideo* iface)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);

    SetRect(&This->SourceRect, 0, 0, This->VideoWidth, This->VideoHeight);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_SetDefaultTargetRect(BaseControlVideo* iface)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    RECT rect;

    if (!GetClientRect(This->baseControlWindow.baseWindow.hWnd, &rect))
        return E_FAIL;

    SetRect(&This->DestRect, 0, 0, rect.right, rect.bottom);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_SetSourceRect(BaseControlVideo* iface, RECT *pSourceRect)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    CopyRect(&This->SourceRect,pSourceRect);
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_SetTargetRect(BaseControlVideo* iface, RECT *pTargetRect)
{
    VideoRendererImpl *This = impl_from_BaseControlVideo(iface);
    CopyRect(&This->DestRect,pTargetRect);
    return S_OK;
}

static const BaseControlVideoFuncTable renderer_BaseControlVideoFuncTable = {
    VideoRenderer_GetSourceRect,
    VideoRenderer_GetStaticImage,
    VideoRenderer_GetTargetRect,
    VideoRenderer_GetVideoFormat,
    VideoRenderer_IsDefaultSourceRect,
    VideoRenderer_IsDefaultTargetRect,
    VideoRenderer_SetDefaultSourceRect,
    VideoRenderer_SetDefaultTargetRect,
    VideoRenderer_SetSourceRect,
    VideoRenderer_SetTargetRect
};

/** IMediaFilter methods **/

static HRESULT WINAPI VideoRenderer_Pause(IBaseFilter * iface)
{
    VideoRendererImpl *This = impl_from_IBaseFilter(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->renderer.csRenderLock);
    if (This->renderer.filter.state != State_Paused)
    {
        if (This->renderer.filter.state == State_Stopped)
        {
            This->renderer.sink.end_of_stream = 0;
            ResetEvent(This->hEvent);
            VideoRenderer_AutoShowWindow(This);
        }

        ResetEvent(This->renderer.flush_event);
        This->renderer.filter.state = State_Paused;
    }
    LeaveCriticalSection(&This->renderer.csRenderLock);

    return S_OK;
}

static const IBaseFilterVtbl VideoRenderer_Vtbl =
{
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    VideoRenderer_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface,
                                                     LONG *FullScreenMode)
{
    VideoRendererImpl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p): %d\n", This, iface, FullScreenMode, This->FullScreenMode);

    if (!FullScreenMode)
        return E_POINTER;

    *FullScreenMode = This->FullScreenMode;

    return S_OK;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface,
                                                     LONG FullScreenMode)
{
    VideoRendererImpl *This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, FullScreenMode);

    if (FullScreenMode) {
        This->baseControlWindow.baseWindow.WindowStyles = GetWindowLongW(This->baseControlWindow.baseWindow.hWnd, GWL_STYLE);
        ShowWindow(This->baseControlWindow.baseWindow.hWnd, SW_HIDE);
        SetParent(This->baseControlWindow.baseWindow.hWnd, 0);
        SetWindowLongW(This->baseControlWindow.baseWindow.hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(This->baseControlWindow.baseWindow.hWnd,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
        GetWindowRect(This->baseControlWindow.baseWindow.hWnd, &This->DestRect);
        This->WindowPos = This->DestRect;
    } else {
        ShowWindow(This->baseControlWindow.baseWindow.hWnd, SW_HIDE);
        SetParent(This->baseControlWindow.baseWindow.hWnd, This->baseControlWindow.hwndOwner);
        SetWindowLongW(This->baseControlWindow.baseWindow.hWnd, GWL_STYLE, This->baseControlWindow.baseWindow.WindowStyles);
        GetClientRect(This->baseControlWindow.baseWindow.hWnd, &This->DestRect);
        SetWindowPos(This->baseControlWindow.baseWindow.hWnd,0,This->DestRect.left,This->DestRect.top,This->DestRect.right,This->DestRect.bottom,SWP_NOZORDER|SWP_SHOWWINDOW);
        This->WindowPos = This->DestRect;
    }
    This->FullScreenMode = FullScreenMode;

    return S_OK;
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    BaseControlWindowImpl_QueryInterface,
    BaseControlWindowImpl_AddRef,
    BaseControlWindowImpl_Release,
    BaseControlWindowImpl_GetTypeInfoCount,
    BaseControlWindowImpl_GetTypeInfo,
    BaseControlWindowImpl_GetIDsOfNames,
    BaseControlWindowImpl_Invoke,
    BaseControlWindowImpl_put_Caption,
    BaseControlWindowImpl_get_Caption,
    BaseControlWindowImpl_put_WindowStyle,
    BaseControlWindowImpl_get_WindowStyle,
    BaseControlWindowImpl_put_WindowStyleEx,
    BaseControlWindowImpl_get_WindowStyleEx,
    BaseControlWindowImpl_put_AutoShow,
    BaseControlWindowImpl_get_AutoShow,
    BaseControlWindowImpl_put_WindowState,
    BaseControlWindowImpl_get_WindowState,
    BaseControlWindowImpl_put_BackgroundPalette,
    BaseControlWindowImpl_get_BackgroundPalette,
    BaseControlWindowImpl_put_Visible,
    BaseControlWindowImpl_get_Visible,
    BaseControlWindowImpl_put_Left,
    BaseControlWindowImpl_get_Left,
    BaseControlWindowImpl_put_Width,
    BaseControlWindowImpl_get_Width,
    BaseControlWindowImpl_put_Top,
    BaseControlWindowImpl_get_Top,
    BaseControlWindowImpl_put_Height,
    BaseControlWindowImpl_get_Height,
    BaseControlWindowImpl_put_Owner,
    BaseControlWindowImpl_get_Owner,
    BaseControlWindowImpl_put_MessageDrain,
    BaseControlWindowImpl_get_MessageDrain,
    BaseControlWindowImpl_get_BorderColor,
    BaseControlWindowImpl_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    BaseControlWindowImpl_SetWindowForeground,
    BaseControlWindowImpl_NotifyOwnerMessage,
    BaseControlWindowImpl_SetWindowPosition,
    BaseControlWindowImpl_GetWindowPosition,
    BaseControlWindowImpl_GetMinIdealImageSize,
    BaseControlWindowImpl_GetMaxIdealImageSize,
    BaseControlWindowImpl_GetRestorePosition,
    BaseControlWindowImpl_HideCursor,
    BaseControlWindowImpl_IsCursorHidden
};

HRESULT VideoRenderer_create(IUnknown *outer, void **out)
{
    static const WCHAR sink_name[] = {'I','n',0};
    HRESULT hr;
    VideoRendererImpl * pVideoRenderer;

    *out = NULL;

    pVideoRenderer = CoTaskMemAlloc(sizeof(VideoRendererImpl));

    pVideoRenderer->init = FALSE;
    ZeroMemory(&pVideoRenderer->SourceRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->DestRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->WindowPos, sizeof(RECT));
    pVideoRenderer->FullScreenMode = OAFALSE;

    hr = strmbase_renderer_init(&pVideoRenderer->renderer, &VideoRenderer_Vtbl,
            outer, &CLSID_VideoRenderer, sink_name,
            (DWORD_PTR)(__FILE__ ": VideoRendererImpl.csFilter"), &BaseFuncTable);

    if (FAILED(hr))
        goto fail;

    hr = BaseControlWindow_Init(&pVideoRenderer->baseControlWindow, &IVideoWindow_VTable,
            &pVideoRenderer->renderer.filter, &pVideoRenderer->renderer.filter.csFilter,
            &pVideoRenderer->renderer.sink.pin, &renderer_BaseWindowFuncTable);
    if (FAILED(hr))
        goto fail;

    hr = strmbase_video_init(&pVideoRenderer->baseControlVideo,
            &pVideoRenderer->renderer.filter, &pVideoRenderer->renderer.filter.csFilter,
            &pVideoRenderer->renderer.sink.pin, &renderer_BaseControlVideoFuncTable);
    if (FAILED(hr))
        goto fail;

    if (!CreateRenderingSubsystem(pVideoRenderer)) {
        hr = E_FAIL;
        goto fail;
    }

    *out = &pVideoRenderer->renderer.filter.IUnknown_inner;
    return S_OK;

fail:
    strmbase_renderer_cleanup(&pVideoRenderer->renderer);
    CoTaskMemFree(pVideoRenderer);
    return hr;
}

HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    /* TODO: Attempt to use the VMR-7 renderer instead when possible */
    return VideoRenderer_create(pUnkOuter, ppv);
}
