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

#include "config.h"

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IBaseFilterVtbl VideoRenderer_Vtbl;
static const IUnknownVtbl IInner_VTable;
static const IBasicVideoVtbl IBasicVideo_VTable;
static const IVideoWindowVtbl IVideoWindow_VTable;
static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl;

typedef struct VideoRendererImpl
{
    BaseRenderer renderer;
    const IBasicVideoVtbl * IBasicVideo_vtbl;
    const IVideoWindowVtbl * IVideoWindow_vtbl;
    const IUnknownVtbl * IInner_vtbl;
    const IAMFilterMiscFlagsVtbl *IAMFilterMiscFlags_vtbl;

    BOOL init;
    HANDLE hThread;

    BaseWindow baseWindow;

    DWORD ThreadID;
    HANDLE hEvent;
/* hEvent == evComplete? */
    BOOL ThreadResult;
    HWND hWndMsgDrain;
    HWND hWndOwner;
    BOOL AutoShow;
    RECT SourceRect;
    RECT DestRect;
    RECT WindowPos;
    LONG VideoWidth;
    LONG VideoHeight;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    LONG WindowStyle;
} VideoRendererImpl;

static inline VideoRendererImpl *impl_from_BaseWindow(BaseWindow *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, baseWindow);
}

static inline VideoRendererImpl *impl_from_BaseRenderer(BaseRenderer *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, renderer);
}

static inline VideoRendererImpl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, VideoRendererImpl, renderer.filter.IBaseFilter_iface);
}

static DWORD WINAPI MessageLoop(LPVOID lpParameter)
{
    VideoRendererImpl* This = lpParameter;
    MSG msg; 
    BOOL fGotMessage;

    TRACE("Starting message loop\n");

    if (FAILED(BaseWindowImpl_PrepareWindow(&This->baseWindow)))
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

static void VideoRenderer_AutoShowWindow(VideoRendererImpl *This) {
    if (!This->init && (!This->WindowPos.right || !This->WindowPos.top))
    {
        DWORD style = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);
        DWORD style_ex = GetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE);

        if (!This->WindowPos.right)
        {
            This->WindowPos.left = This->SourceRect.left;
            This->WindowPos.right = This->SourceRect.right;
        }
        if (!This->WindowPos.bottom)
        {
            This->WindowPos.top = This->SourceRect.top;
            This->WindowPos.bottom = This->SourceRect.bottom;
        }

        AdjustWindowRectEx(&This->WindowPos, style, TRUE, style_ex);

        TRACE("WindowPos: %d %d %d %d\n", This->WindowPos.left, This->WindowPos.top, This->WindowPos.right, This->WindowPos.bottom);
        SetWindowPos(This->baseWindow.hWnd, NULL,
            This->WindowPos.left,
            This->WindowPos.top,
            This->WindowPos.right - This->WindowPos.left,
            This->WindowPos.bottom - This->WindowPos.top,
            SWP_NOZORDER|SWP_NOMOVE|SWP_DEFERERASE);

        GetClientRect(This->baseWindow.hWnd, &This->DestRect);
    }
    else if (!This->init)
        This->DestRect = This->WindowPos;
    This->init = TRUE;
    if (This->AutoShow)
        ShowWindow(This->baseWindow.hWnd, SW_SHOW);
}

static DWORD VideoRenderer_SendSampleData(VideoRendererImpl* This, LPBYTE data, DWORD size)
{
    AM_MEDIA_TYPE amt;
    HRESULT hr = S_OK;
    DDSURFACEDESC sdesc;
    BITMAPINFOHEADER *bmiHeader;

    TRACE("(%p)->(%p, %d)\n", This, data, size);

    sdesc.dwSize = sizeof(sdesc);
    hr = IPin_ConnectionMediaType(&This->renderer.pInputPin->pin.IPin_iface, &amt);
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

    if (!This->baseWindow.hDC) {
        ERR("Cannot get DC from window!\n");
        return E_FAIL;
    }

    TRACE("Src Rect: %d %d %d %d\n", This->SourceRect.left, This->SourceRect.top, This->SourceRect.right, This->SourceRect.bottom);
    TRACE("Dst Rect: %d %d %d %d\n", This->DestRect.left, This->DestRect.top, This->DestRect.right, This->DestRect.bottom);

    StretchDIBits(This->baseWindow.hDC, This->DestRect.left, This->DestRect.top, This->DestRect.right -This->DestRect.left,
                  This->DestRect.bottom - This->DestRect.top, This->SourceRect.left, This->SourceRect.top,
                  This->SourceRect.right - This->SourceRect.left, This->SourceRect.bottom - This->SourceRect.top,
                  data, (BITMAPINFO *)bmiHeader, DIB_RGB_COLORS, SRCCOPY);

    return S_OK;
}

HRESULT WINAPI VideoRenderer_ShouldDrawSampleNow(BaseRenderer *This, IMediaSample *pSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime)
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
        LeaveCriticalSection(iface->pInputPin->pin.pCritSec);
        LeaveCriticalSection(&iface->filter.csFilter);
        LeaveCriticalSection(&iface->csRenderLock);
        WaitForSingleObject(This->hEvent, INFINITE);
        EnterCriticalSection(&iface->csRenderLock);
        EnterCriticalSection(&iface->filter.csFilter);
        EnterCriticalSection(iface->pInputPin->pin.pCritSec);
    }
    if (This->renderer.filter.state == State_Paused) {
        ResetEvent(This->hEvent);
    }

    return BaseRendererImpl_EndFlush(iface);
}

static VOID WINAPI VideoRenderer_OnStopStreaming(BaseRenderer* iface)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)->()\n", This);

    SetEvent(This->hEvent);
    if (This->AutoShow)
        /* Black it out */
        RedrawWindow(This->baseWindow.hWnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE);
}

static VOID WINAPI VideoRenderer_OnStartStreaming(BaseRenderer* iface)
{
    VideoRendererImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)\n", This);

    if (This->renderer.pInputPin->pin.pConnectedTo && (This->renderer.filter.state == State_Stopped || !This->renderer.pInputPin->end_of_stream))
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

static BOOL WINAPI VideoRenderer_PossiblyEatMessage(BaseWindow *iface, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VideoRendererImpl *This = impl_from_BaseWindow(iface);

    if (This->hWndMsgDrain)
    {
        switch(uMsg)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEACTIVATE:
            case WM_MOUSEMOVE:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMOUSEMOVE:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                PostMessageW(This->hWndMsgDrain, uMsg, wParam, lParam);
                return TRUE;
                break;
            default:
                break;
        }
    }
    return FALSE;
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
    return TRUE;
}

static const BaseRendererFuncTable BaseFuncTable = {
    VideoRenderer_CheckMediaType,
    VideoRenderer_DoRenderSample,
    /**/
    NULL,
    NULL,
    NULL,
    VideoRenderer_OnStartStreaming,
    VideoRenderer_OnStopStreaming,
    NULL,
    NULL,
    NULL,
    VideoRenderer_ShouldDrawSampleNow,
    NULL,
    /**/
    NULL,
    NULL,
    NULL,
    NULL,
    VideoRenderer_EndFlush,
};

static const BaseWindowFuncTable renderer_BaseWindowFuncTable = {
    VideoRenderer_GetClassWindowStyles,
    NULL,
    NULL,
    VideoRenderer_PossiblyEatMessage,
    VideoRenderer_OnSize
};

HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    VideoRendererImpl * pVideoRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    pVideoRenderer = CoTaskMemAlloc(sizeof(VideoRendererImpl));
    pVideoRenderer->pUnkOuter = pUnkOuter;
    pVideoRenderer->bUnkOuterValid = FALSE;
    pVideoRenderer->bAggregatable = FALSE;
    pVideoRenderer->IInner_vtbl = &IInner_VTable;
    pVideoRenderer->IAMFilterMiscFlags_vtbl = &IAMFilterMiscFlags_Vtbl;
    pVideoRenderer->IBasicVideo_vtbl = &IBasicVideo_VTable;
    pVideoRenderer->IVideoWindow_vtbl = &IVideoWindow_VTable;

    pVideoRenderer->init = 0;
    pVideoRenderer->AutoShow = 1;
    ZeroMemory(&pVideoRenderer->SourceRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->DestRect, sizeof(RECT));
    ZeroMemory(&pVideoRenderer->WindowPos, sizeof(RECT));
    pVideoRenderer->hWndMsgDrain = pVideoRenderer->hWndOwner = NULL;
    pVideoRenderer->WindowStyle = WS_OVERLAPPED;

    hr = BaseRenderer_Init(&pVideoRenderer->renderer, &VideoRenderer_Vtbl, pUnkOuter, &CLSID_VideoRenderer, (DWORD_PTR)(__FILE__ ": VideoRendererImpl.csFilter"), &BaseFuncTable);

    if (FAILED(hr))
        goto fail;

    *ppv = pVideoRenderer;
    hr = BaseWindow_Init(&pVideoRenderer->baseWindow, &renderer_BaseWindowFuncTable);
    if (FAILED(hr))
        goto fail;

    if (!CreateRenderingSubsystem(pVideoRenderer))
        return E_FAIL;

    return hr;
fail:
    BaseRendererImpl_Release(&pVideoRenderer->renderer.filter.IBaseFilter_iface);
    CoTaskMemFree(pVideoRenderer);
    return hr;
}

HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    /* TODO: Attempt to use the VMR-7 renderer instead when possible */
    return VideoRenderer_create(pUnkOuter, ppv);
}

static HRESULT WINAPI VideoRendererInner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IInner_vtbl;
    else if (IsEqualIID(riid, &IID_IBasicVideo))
        *ppv = &This->IBasicVideo_vtbl;
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        *ppv = &This->IVideoWindow_vtbl;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_vtbl;
    else
    {
        HRESULT hr;
        hr = BaseRendererImpl_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
        if (SUCCEEDED(hr))
            return hr;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VideoRendererInner_AddRef(IUnknown * iface)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    ULONG refCount = BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VideoRendererInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(VideoRendererImpl, IInner_vtbl, iface);
    ULONG refCount = BaseRendererImpl_Release(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        BaseWindowImpl_DoneWithWindow(&This->baseWindow);
        PostThreadMessageW(This->ThreadID, WM_QUIT, 0, 0);
        WaitForSingleObject(This->hThread, INFINITE);
        CloseHandle(This->hThread);
        CloseHandle(This->hEvent);

        TRACE("Destroying Video Renderer\n");
        CoTaskMemFree(This);

        return 0;
    }
    else
        return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    VideoRendererInner_QueryInterface,
    VideoRendererInner_AddRef,
    VideoRendererInner_Release
};

static HRESULT WINAPI VideoRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VideoRendererImpl *This = impl_from_IBaseFilter(iface);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG WINAPI VideoRenderer_AddRef(IBaseFilter * iface)
{
    VideoRendererImpl *This = impl_from_IBaseFilter(iface);

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG WINAPI VideoRenderer_Release(IBaseFilter * iface)
{
    VideoRendererImpl *This = impl_from_IBaseFilter(iface);

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

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
            This->renderer.pInputPin->end_of_stream = 0;
            ResetEvent(This->hEvent);
            VideoRenderer_AutoShowWindow(This);
        }

        ResetEvent(This->renderer.RenderEvent);
        This->renderer.filter.state = State_Paused;
    }
    LeaveCriticalSection(&This->renderer.csRenderLock);

    return S_OK;
}

static const IBaseFilterVtbl VideoRenderer_Vtbl =
{
    VideoRenderer_QueryInterface,
    VideoRenderer_AddRef,
    VideoRenderer_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    VideoRenderer_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseRendererImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VideoRenderer_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release(&This->renderer.filter.IBaseFilter_iface);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Basicvideo_GetTypeInfoCount(IBasicVideo *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetTypeInfo(IBasicVideo *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetIDsOfNames(IBasicVideo *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_Invoke(IBasicVideo *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI Basicvideo_get_AvgTimePerFrame(IBasicVideo *iface,
						     REFTIME *pAvgTimePerFrame) {
    AM_MEDIA_TYPE *pmt;
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    if (!This->renderer.pInputPin->pin.pConnectedTo)
        return VFW_E_NOT_CONNECTED;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    pmt = &This->renderer.pInputPin->pin.mtCurrent;
    if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)) {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)pmt->pbFormat;
        *pAvgTimePerFrame = vih->AvgTimePerFrame;
    } else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2)) {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
        *pAvgTimePerFrame = vih->AvgTimePerFrame;
    } else {
        ERR("Unknown format type %s\n", qzdebugstr_guid(&pmt->formattype));
        *pAvgTimePerFrame = 0;
    }
    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitRate(IBasicVideo *iface,
                                             LONG *pBitRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitErrorRate(IBasicVideo *iface,
                                                  LONG *pBitErrorRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitErrorRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoWidth(IBasicVideo *iface,
                                                LONG *pVideoWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    *pVideoWidth = This->VideoWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoHeight(IBasicVideo *iface,
                                                 LONG *pVideoHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    *pVideoHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceLeft(IBasicVideo *iface,
                                                LONG SourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceLeft);

    This->SourceRect.left = SourceLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceLeft(IBasicVideo *iface,
                                                LONG *pSourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    *pSourceLeft = This->SourceRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceWidth(IBasicVideo *iface,
                                                 LONG SourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceWidth);

    This->SourceRect.right = This->SourceRect.left + SourceWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceWidth(IBasicVideo *iface,
                                                 LONG *pSourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    *pSourceWidth = This->SourceRect.right - This->SourceRect.left;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceTop(IBasicVideo *iface,
                                               LONG SourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceTop);

    This->SourceRect.top = SourceTop;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceTop(IBasicVideo *iface,
                                               LONG *pSourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    *pSourceTop = This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceHeight(IBasicVideo *iface,
                                                  LONG SourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceHeight);

    This->SourceRect.bottom = This->SourceRect.top + SourceHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceHeight(IBasicVideo *iface,
                                                  LONG *pSourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    *pSourceHeight = This->SourceRect.bottom - This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationLeft(IBasicVideo *iface,
                                                     LONG DestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationLeft);

    This->DestRect.left = DestinationLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationLeft(IBasicVideo *iface,
                                                     LONG *pDestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    *pDestinationLeft = This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationWidth(IBasicVideo *iface,
                                                      LONG DestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationWidth);

    This->DestRect.right = This->DestRect.left + DestinationWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationWidth(IBasicVideo *iface,
                                                      LONG *pDestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    *pDestinationWidth = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationTop(IBasicVideo *iface,
                                                    LONG DestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationTop);

    This->DestRect.top = DestinationTop;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationTop(IBasicVideo *iface,
                                                    LONG *pDestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    *pDestinationTop = This->DestRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationHeight(IBasicVideo *iface,
                                                       LONG DestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationHeight);

    This->DestRect.right = This->DestRect.left + DestinationHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationHeight(IBasicVideo *iface,
                                                       LONG *pDestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    *pDestinationHeight = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetSourcePosition(IBasicVideo *iface,
                                                   LONG Left,
                                                   LONG Top,
                                                   LONG Width,
                                                   LONG Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    This->SourceRect.left = Left;
    This->SourceRect.top = Top;
    This->SourceRect.right = Left + Width;
    This->SourceRect.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetSourcePosition(IBasicVideo *iface,
                                                   LONG *pLeft,
                                                   LONG *pTop,
                                                   LONG *pWidth,
                                                   LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->SourceRect.left;
    *pTop = This->SourceRect.top;
    *pWidth = This->SourceRect.right - This->SourceRect.left;
    *pHeight = This->SourceRect.bottom - This->SourceRect.top;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultSourcePosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    This->SourceRect.left = 0;
    This->SourceRect.top = 0;
    This->SourceRect.right = This->VideoWidth;
    This->SourceRect.bottom = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDestinationPosition(IBasicVideo *iface,
                                                        LONG Left,
                                                        LONG Top,
                                                        LONG Width,
                                                        LONG Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    This->DestRect.left = Left;
    This->DestRect.top = Top;
    This->DestRect.right = Left + Width;
    This->DestRect.bottom = Top + Height;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetDestinationPosition(IBasicVideo *iface,
                                                        LONG *pLeft,
                                                        LONG *pTop,
                                                        LONG *pWidth,
                                                        LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->DestRect.left;
    *pTop = This->DestRect.top;
    *pWidth = This->DestRect.right - This->DestRect.left;
    *pHeight = This->DestRect.bottom - This->DestRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetDefaultDestinationPosition(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);
    RECT rect;

    TRACE("(%p/%p)->()\n", This, iface);

    if (!GetClientRect(This->baseWindow.hWnd, &rect))
        return E_FAIL;
    
    This->SourceRect.left = 0;
    This->SourceRect.top = 0;
    This->SourceRect.right = rect.right;
    This->SourceRect.bottom = rect.bottom;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoSize(IBasicVideo *iface,
                                              LONG *pWidth,
                                              LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoPaletteEntries(IBasicVideo *iface,
                                                        LONG StartIndex,
                                                        LONG Entries,
                                                        LONG *pRetrieved,
                                                        LONG *pPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p, %p)\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    if (pRetrieved)
        *pRetrieved = 0;
    return VFW_E_NO_PALETTE_AVAILABLE;
}

static HRESULT WINAPI Basicvideo_GetCurrentImage(IBasicVideo *iface,
                                                 LONG *pBufferSize,
                                                 LONG *pDIBImage) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);
    BITMAPINFOHEADER *bmiHeader;
    LONG needed_size;
    AM_MEDIA_TYPE *amt = &This->renderer.pInputPin->pin.mtCurrent;
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

static HRESULT WINAPI Basicvideo_IsUsingDefaultSource(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_IsUsingDefaultDestination(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IBasicVideoVtbl IBasicVideo_VTable =
{
    Basicvideo_QueryInterface,
    Basicvideo_AddRef,
    Basicvideo_Release,
    Basicvideo_GetTypeInfoCount,
    Basicvideo_GetTypeInfo,
    Basicvideo_GetIDsOfNames,
    Basicvideo_Invoke,
    Basicvideo_get_AvgTimePerFrame,
    Basicvideo_get_BitRate,
    Basicvideo_get_BitErrorRate,
    Basicvideo_get_VideoWidth,
    Basicvideo_get_VideoHeight,
    Basicvideo_put_SourceLeft,
    Basicvideo_get_SourceLeft,
    Basicvideo_put_SourceWidth,
    Basicvideo_get_SourceWidth,
    Basicvideo_put_SourceTop,
    Basicvideo_get_SourceTop,
    Basicvideo_put_SourceHeight,
    Basicvideo_get_SourceHeight,
    Basicvideo_put_DestinationLeft,
    Basicvideo_get_DestinationLeft,
    Basicvideo_put_DestinationWidth,
    Basicvideo_get_DestinationWidth,
    Basicvideo_put_DestinationTop,
    Basicvideo_get_DestinationTop,
    Basicvideo_put_DestinationHeight,
    Basicvideo_get_DestinationHeight,
    Basicvideo_SetSourcePosition,
    Basicvideo_GetSourcePosition,
    Basicvideo_SetDefaultSourcePosition,
    Basicvideo_SetDestinationPosition,
    Basicvideo_GetDestinationPosition,
    Basicvideo_SetDefaultDestinationPosition,
    Basicvideo_GetVideoSize,
    Basicvideo_GetVideoPaletteEntries,
    Basicvideo_GetCurrentImage,
    Basicvideo_IsUsingDefaultSource,
    Basicvideo_IsUsingDefaultDestination
};


/*** IUnknown methods ***/
static HRESULT WINAPI Videowindow_QueryInterface(IVideoWindow *iface,
						 REFIID riid,
						 LPVOID*ppvObj) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VideoRenderer_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release(&This->renderer.filter.IBaseFilter_iface);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Videowindow_GetTypeInfoCount(IVideoWindow *iface,
						   UINT*pctinfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetTypeInfo(IVideoWindow *iface,
					      UINT iTInfo,
					      LCID lcid,
					      ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetIDsOfNames(IVideoWindow *iface,
						REFIID riid,
						LPOLESTR*rgszNames,
						UINT cNames,
						LCID lcid,
						DISPID*rgDispId) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Videowindow_Invoke(IVideoWindow *iface,
					 DISPID dispIdMember,
					 REFIID riid,
					 LCID lcid,
					 WORD wFlags,
					 DISPPARAMS*pDispParams,
					 VARIANT*pVarResult,
					 EXCEPINFO*pExepInfo,
					 UINT*puArgErr) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IVideoWindow methods ***/
static HRESULT WINAPI Videowindow_put_Caption(IVideoWindow *iface,
					      BSTR strCaption) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    if (!SetWindowTextW(This->baseWindow.hWnd, strCaption))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    GetWindowTextW(This->baseWindow.hWnd, (LPWSTR)strCaption, 100);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyle(IVideoWindow *iface,
                                                  LONG WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    LONG old;

    old = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);

    TRACE("(%p/%p)->(%x -> %x)\n", This, iface, old, WindowStyle);

    if (WindowStyle & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;

    SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, WindowStyle);
    SetWindowPos(This->baseWindow.hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOZORDER);
    This->WindowStyle = WindowStyle;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyle(IVideoWindow *iface,
                                                  LONG *WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    *WindowStyle = This->WindowStyle;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyleEx(IVideoWindow *iface,
                                                    LONG WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyleEx);

    if (!SetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE, WindowStyleEx))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyleEx(IVideoWindow *iface,
                                                    LONG *WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    *WindowStyleEx = GetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_AutoShow(IVideoWindow *iface,
                                               LONG AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, AutoShow);

    This->AutoShow = AutoShow;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_AutoShow(IVideoWindow *iface,
                                               LONG *AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    *AutoShow = This->AutoShow;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowState(IVideoWindow *iface,
                                                  LONG WindowState) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowState);
    ShowWindow(This->baseWindow.hWnd, WindowState);
    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowState(IVideoWindow *iface,
                                                  LONG *WindowState) {
    WINDOWPLACEMENT place;
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    place.length = sizeof(place);
    GetWindowPlacement(This->baseWindow.hWnd, &place);
    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);
    *WindowState = place.showCmd;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BackgroundPalette(IVideoWindow *iface,
                                                        LONG BackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BackgroundPalette(IVideoWindow *iface,
                                                        LONG *pBackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Visible(IVideoWindow *iface,
                                              LONG Visible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Visible);

    ShowWindow(This->baseWindow.hWnd, Visible ? SW_SHOW : SW_HIDE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Visible(IVideoWindow *iface,
                                              LONG *pVisible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    *pVisible = IsWindowVisible(This->baseWindow.hWnd);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Left(IVideoWindow *iface,
                                           LONG Left) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Left);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, This->WindowPos.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.left = Left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Left(IVideoWindow *iface,
                                           LONG *pLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    *pLeft = This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Width(IVideoWindow *iface,
                                            LONG Width) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Width);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, Width, This->WindowPos.bottom-This->WindowPos.top, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.right = This->WindowPos.left + Width;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Width(IVideoWindow *iface,
                                            LONG *pWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    *pWidth = This->WindowPos.right - This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Top(IVideoWindow *iface,
                                          LONG Top) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Top);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, This->WindowPos.left, Top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.top = Top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Top(IVideoWindow *iface,
                                          LONG *pTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    *pTop = This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Height(IVideoWindow *iface,
                                             LONG Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, This->WindowPos.right-This->WindowPos.left, Height, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.bottom = This->WindowPos.top + Height;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Height(IVideoWindow *iface,
                                             LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    This->hWndOwner = (HWND)Owner;
    SetParent(This->baseWindow.hWnd, This->hWndOwner);
    if (This->WindowStyle & WS_CHILD)
    {
        LONG old = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);
        if (old != This->WindowStyle)
        {
            SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, This->WindowStyle);
            SetWindowPos(This->baseWindow.hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOZORDER);
        }
    }

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    *(HWND*)Owner = This->hWndOwner;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_MessageDrain(IVideoWindow *iface,
						   OAHWND Drain) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    This->hWndMsgDrain = (HWND)Drain;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_MessageDrain(IVideoWindow *iface,
						   OAHWND *Drain) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    *Drain = (OAHWND)This->hWndMsgDrain;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BorderColor(IVideoWindow *iface,
                                                  LONG *Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BorderColor(IVideoWindow *iface,
                                                  LONG Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_FullScreenMode(IVideoWindow *iface,
                                                     LONG *FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_FullScreenMode(IVideoWindow *iface,
                                                     LONG FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, FullScreenMode);

    if (FullScreenMode) {
        ShowWindow(This->baseWindow.hWnd, SW_HIDE);
        SetParent(This->baseWindow.hWnd, 0);
        SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(This->baseWindow.hWnd,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
        GetWindowRect(This->baseWindow.hWnd, &This->DestRect);
        This->WindowPos = This->DestRect;
    } else {
        ShowWindow(This->baseWindow.hWnd, SW_HIDE);
        SetParent(This->baseWindow.hWnd, This->hWndOwner);
        SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, This->WindowStyle);
        GetClientRect(This->baseWindow.hWnd, &This->DestRect);
        SetWindowPos(This->baseWindow.hWnd,0,This->DestRect.left,This->DestRect.top,This->DestRect.right,This->DestRect.bottom,SWP_NOZORDER|SWP_SHOWWINDOW);
        This->WindowPos = This->DestRect;
    }

    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowForeground(IVideoWindow *iface,
                                                      LONG Focus) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    BOOL ret;
    IPin* pPin;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Focus);

    if ((Focus != FALSE) && (Focus != TRUE))
        return E_INVALIDARG;

    hr = IPin_ConnectedTo(&This->renderer.pInputPin->pin.IPin_iface, &pPin);
    if ((hr != S_OK) || !pPin)
        return VFW_E_NOT_CONNECTED;

    if (Focus)
        ret = SetForegroundWindow(This->baseWindow.hWnd);
    else
        ret = SetWindowPos(This->baseWindow.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

    if (!ret)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
                                                     LONG uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08lx, %d, %08lx, %08lx)\n", This, iface, hwnd, uMsg, wParam, lParam);

    if (!PostMessageW(This->baseWindow.hWnd, uMsg, wParam, lParam))
        return E_FAIL;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowPosition(IVideoWindow *iface,
                                                    LONG Left,
                                                    LONG Top,
                                                    LONG Width,
                                                    LONG Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, Top, Width, Height, SWP_NOZORDER))
        return E_FAIL;

    This->WindowPos.left = Left;
    This->WindowPos.top = Top;
    This->WindowPos.right = Left + Width;
    This->WindowPos.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_GetWindowPosition(IVideoWindow *iface,
                                                    LONG *pLeft,
                                                    LONG *pTop,
                                                    LONG *pWidth,
                                                    LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->WindowPos.left;
    *pTop = This->WindowPos.top;
    *pWidth = This->WindowPos.right - This->WindowPos.left;
    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMinIdealImageSize(IVideoWindow *iface,
                                                       LONG *pWidth,
                                                       LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMaxIdealImageSize(IVideoWindow *iface,
                                                       LONG *pWidth,
                                                       LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetRestorePosition(IVideoWindow *iface,
                                                     LONG *pLeft,
                                                     LONG *pTop,
                                                     LONG *pWidth,
                                                     LONG *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_HideCursor(IVideoWindow *iface,
                                             LONG HideCursor) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

static HRESULT WINAPI Videowindow_IsCursorHidden(IVideoWindow *iface,
                                                 LONG *CursorHidden) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, CursorHidden);

    return S_OK;
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    Videowindow_QueryInterface,
    Videowindow_AddRef,
    Videowindow_Release,
    Videowindow_GetTypeInfoCount,
    Videowindow_GetTypeInfo,
    Videowindow_GetIDsOfNames,
    Videowindow_Invoke,
    Videowindow_put_Caption,
    Videowindow_get_Caption,
    Videowindow_put_WindowStyle,
    Videowindow_get_WindowStyle,
    Videowindow_put_WindowStyleEx,
    Videowindow_get_WindowStyleEx,
    Videowindow_put_AutoShow,
    Videowindow_get_AutoShow,
    Videowindow_put_WindowState,
    Videowindow_get_WindowState,
    Videowindow_put_BackgroundPalette,
    Videowindow_get_BackgroundPalette,
    Videowindow_put_Visible,
    Videowindow_get_Visible,
    Videowindow_put_Left,
    Videowindow_get_Left,
    Videowindow_put_Width,
    Videowindow_get_Width,
    Videowindow_put_Top,
    Videowindow_get_Top,
    Videowindow_put_Height,
    Videowindow_get_Height,
    Videowindow_put_Owner,
    Videowindow_get_Owner,
    Videowindow_put_MessageDrain,
    Videowindow_get_MessageDrain,
    Videowindow_get_BorderColor,
    Videowindow_put_BorderColor,
    Videowindow_get_FullScreenMode,
    Videowindow_put_FullScreenMode,
    Videowindow_SetWindowForeground,
    Videowindow_NotifyOwnerMessage,
    Videowindow_SetWindowPosition,
    Videowindow_GetWindowPosition,
    Videowindow_GetMinIdealImageSize,
    Videowindow_GetMaxIdealImageSize,
    Videowindow_GetRestorePosition,
    Videowindow_HideCursor,
    Videowindow_IsCursorHidden
};

static VideoRendererImpl *from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface) {
    return (VideoRendererImpl*)((char*)iface - offsetof(VideoRendererImpl, IAMFilterMiscFlags_vtbl));
}

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv) {
    VideoRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    VideoRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    VideoRendererImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_Release((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};
