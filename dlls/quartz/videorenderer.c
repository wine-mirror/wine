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
#include "control_private.h"
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

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static BOOL wnd_class_registered = FALSE;

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl VideoRenderer_Vtbl;
static const IBasicVideoVtbl IBasicVideo_VTable;
static const IVideoWindowVtbl IVideoWindow_VTable;
static const IPinVtbl VideoRenderer_InputPin_Vtbl;

typedef struct VideoRendererImpl
{
    const IBaseFilterVtbl * lpVtbl;
    const IBasicVideoVtbl * IBasicVideo_vtbl;
    const IVideoWindowVtbl * IVideoWindow_vtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    InputPin * pInputPin;
    IPin ** ppPins;

    BOOL init;
    HANDLE hThread;
    DWORD ThreadID;
    HANDLE hEvent;
    BOOL ThreadResult;
    HWND hWnd;
    HWND hWndMsgDrain;
    BOOL AutoShow;
    RECT SourceRect;
    RECT DestRect;
    RECT WindowPos;
    long VideoWidth;
    long VideoHeight;
} VideoRendererImpl;

static LRESULT CALLBACK VideoWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VideoRendererImpl* pVideoRenderer = (VideoRendererImpl*)GetWindowLongA(hwnd, 0);
    LPRECT lprect = (LPRECT)lParam;

    if (pVideoRenderer && pVideoRenderer->hWndMsgDrain)
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
                PostMessageA(pVideoRenderer->hWndMsgDrain, uMsg, wParam, lParam);
                break;
            default:
                break;
        }
    }

    switch(uMsg)
    {
        case WM_SIZING:
            /* TRACE("WM_SIZING %d %d %d %d\n", lprect->left, lprect->top, lprect->right, lprect->bottom); */
            SetWindowPos(hwnd, NULL, lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top, SWP_NOZORDER);
            GetClientRect(hwnd, &pVideoRenderer->DestRect);
            return TRUE;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL CreateRenderingWindow(VideoRendererImpl* This)
{
    WNDCLASSA winclass;

    TRACE("(%p)->()\n", This);
    
    winclass.style = 0;
    winclass.lpfnWndProc = VideoWndProcA;
    winclass.cbClsExtra = 0;
    winclass.cbWndExtra = sizeof(VideoRendererImpl*);
    winclass.hInstance = NULL;
    winclass.hIcon = NULL;
    winclass.hCursor = NULL;
    winclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    winclass.lpszMenuName = NULL;
    winclass.lpszClassName = "Wine ActiveMovie Class";

    if (!wnd_class_registered)
    {
        if (!RegisterClassA(&winclass))
        {
            ERR("Unable to register window %u\n", GetLastError());
            return FALSE;
        }
        wnd_class_registered = TRUE;
    }

    This->hWnd = CreateWindowExA(0, "Wine ActiveMovie Class", "Wine ActiveMovie Window", WS_SIZEBOX,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
                                 NULL, NULL, NULL);

    if (!This->hWnd)
    {
        ERR("Unable to create window\n");
        return FALSE;
    }

    SetWindowLongA(This->hWnd, 0, (LONG)This);

    return TRUE;
}

static DWORD WINAPI MessageLoop(LPVOID lpParameter)
{
    VideoRendererImpl* This = (VideoRendererImpl*) lpParameter;
    MSG msg; 
    BOOL fGotMessage;

    TRACE("Starting message loop\n");

    if (!CreateRenderingWindow(This))
    {
        This->ThreadResult = FALSE;
        SetEvent(This->hEvent);
        return 0;
    }

    This->ThreadResult = TRUE;
    SetEvent(This->hEvent);

    while ((fGotMessage = GetMessageA(&msg, NULL, 0, 0)) != 0 && fGotMessage != -1) 
    {
        TranslateMessage(&msg); 
        DispatchMessageA(&msg); 
    }

    TRACE("End of message loop\n");

    return msg.wParam;
}

static BOOL CreateRenderingSubsystem(VideoRendererImpl* This)
{
    This->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!This->hEvent)
        return FALSE;

    This->hThread = CreateThread(NULL, 0, MessageLoop, (LPVOID)This, 0, &This->ThreadID);
    if (!This->hThread)
    {
        CloseHandle(This->hEvent);
        return FALSE;
    }

    WaitForSingleObject(This->hEvent, INFINITE);
    CloseHandle(This->hEvent);

    if (!This->ThreadResult)
    {
        CloseHandle(This->hThread);
        return FALSE;
    }

    return TRUE;
}

static const IMemInputPinVtbl MemInputPin_Vtbl = 
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

static HRESULT VideoRenderer_InputPin_Construct(const PIN_INFO * pPinInfo, SAMPLEPROC pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    InputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(InputPin_Init(pPinInfo, pSampleProc, pUserData, pQueryAccept, pCritSec, pPinImpl)))
    {
        pPinImpl->pin.lpVtbl = &VideoRenderer_InputPin_Vtbl;
        pPinImpl->lpVtblMemInput = &MemInputPin_Vtbl;
      
        *ppPin = (IPin *)(&pPinImpl->pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

static DWORD VideoRenderer_SendSampleData(VideoRendererImpl* This, LPBYTE data, DWORD size)
{
    VIDEOINFOHEADER* format;
    AM_MEDIA_TYPE amt;
    HRESULT hr = S_OK;
    DDSURFACEDESC sdesc;
    int width;
    int height;
    LPBYTE palette = NULL;
    HDC hDC;

    TRACE("%p %p %d\n", This, data, size);

    sdesc.dwSize = sizeof(sdesc);
    hr = IPin_ConnectionMediaType(This->ppPins[0], &amt);
    if (FAILED(hr)) {
        ERR("Unable to retrieve media type\n");
        return hr;
    }
    format = (VIDEOINFOHEADER*)amt.pbFormat;

    TRACE("biSize = %d\n", format->bmiHeader.biSize);
    TRACE("biWidth = %d\n", format->bmiHeader.biWidth);
    TRACE("biHeight = %d\n", format->bmiHeader.biHeight);
    TRACE("biPlanes = %d\n", format->bmiHeader.biPlanes);
    TRACE("biBitCount = %d\n", format->bmiHeader.biBitCount);
    TRACE("biCompression = %s\n", debugstr_an((LPSTR)&(format->bmiHeader.biCompression), 4));
    TRACE("biSizeImage = %d\n", format->bmiHeader.biSizeImage);

    width = format->bmiHeader.biWidth;
    height = format->bmiHeader.biHeight;
    palette = ((LPBYTE)&format->bmiHeader) + format->bmiHeader.biSize;
 
    if (!This->init)
    {
        /* Compute the size of the whole window so the client area size matches video one */
        RECT wrect, crect;
        int h, v;
        GetWindowRect(This->hWnd, &wrect);
        GetClientRect(This->hWnd, &crect);
        h = (wrect.right - wrect.left) - (crect.right - crect.left);
        v = (wrect.bottom - wrect.top) - (crect.bottom - crect.top);
        SetWindowPos(This->hWnd, NULL, 0, 0, width + h +20, height + v+20, SWP_NOZORDER|SWP_NOMOVE);
        This->WindowPos.left = 0;
        This->WindowPos.top = 0;
        This->WindowPos.right = width;
        This->WindowPos.bottom = abs(height);
        GetClientRect(This->hWnd, &This->DestRect);
        This->init  = TRUE;
    }

    hDC = GetDC(This->hWnd);

    if (!hDC) {
        ERR("Cannot get DC from window!\n");
        return E_FAIL;
    }

    TRACE("Src Rect: %d %d %d %d\n", This->SourceRect.left, This->SourceRect.top, This->SourceRect.right, This->SourceRect.bottom);
    TRACE("Dst Rect: %d %d %d %d\n", This->DestRect.left, This->DestRect.top, This->DestRect.right, This->DestRect.bottom);

    StretchDIBits(hDC, This->DestRect.left, This->DestRect.top, This->DestRect.right -This->DestRect.left,
                  This->DestRect.bottom - This->DestRect.top, This->SourceRect.left, This->SourceRect.top,
                  This->SourceRect.right - This->SourceRect.left, This->SourceRect.bottom - This->SourceRect.top,
                  data, (BITMAPINFO*)&format->bmiHeader, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(This->hWnd, hDC);
    
    if (This->AutoShow)
        ShowWindow(This->hWnd, SW_SHOW);

    return S_OK;
}

static HRESULT VideoRenderer_Sample(LPVOID iface, IMediaSample * pSample)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);
    
    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        ERR("Cannot get sample time (%x)\n", hr);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("val %p %ld\n", pbSrcStream, cbSrcStream);

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
    
    VideoRenderer_SendSampleData(This, pbSrcStream, cbSrcStream);

    return S_OK;
}

static HRESULT VideoRenderer_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB32) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB24) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB565) ||
        IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_RGB8))
    {
        VideoRendererImpl* This = (VideoRendererImpl*) iface;
        VIDEOINFOHEADER* format = (VIDEOINFOHEADER*)pmt->pbFormat;

        if (!IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
        {
            WARN("Format type %s not supported\n", debugstr_guid(&pmt->formattype));
            return S_FALSE;
        }
        This->SourceRect.left = 0;
        This->SourceRect.top = 0;
        This->SourceRect.right = This->VideoWidth = format->bmiHeader.biWidth;
        This->SourceRect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    VideoRendererImpl * pVideoRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    pVideoRenderer = CoTaskMemAlloc(sizeof(VideoRendererImpl));

    pVideoRenderer->lpVtbl = &VideoRenderer_Vtbl;
    pVideoRenderer->IBasicVideo_vtbl = &IBasicVideo_VTable;
    pVideoRenderer->IVideoWindow_vtbl = &IVideoWindow_VTable;
    
    pVideoRenderer->refCount = 1;
    InitializeCriticalSection(&pVideoRenderer->csFilter);
    pVideoRenderer->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": VideoRendererImpl.csFilter");
    pVideoRenderer->state = State_Stopped;
    pVideoRenderer->pClock = NULL;
    pVideoRenderer->init = 0;
    pVideoRenderer->AutoShow = 1;
    ZeroMemory(&pVideoRenderer->filterInfo, sizeof(FILTER_INFO));

    pVideoRenderer->ppPins = CoTaskMemAlloc(1 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pVideoRenderer;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = VideoRenderer_InputPin_Construct(&piInput, VideoRenderer_Sample, (LPVOID)pVideoRenderer, VideoRenderer_QueryAccept, &pVideoRenderer->csFilter, (IPin **)&pVideoRenderer->pInputPin);

    if (SUCCEEDED(hr))
    {
        pVideoRenderer->ppPins[0] = (IPin *)pVideoRenderer->pInputPin;
        *ppv = (LPVOID)pVideoRenderer;
    }
    else
    {
        CoTaskMemFree(pVideoRenderer->ppPins);
        pVideoRenderer->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&pVideoRenderer->csFilter);
        CoTaskMemFree(pVideoRenderer);
    }

    if (!CreateRenderingSubsystem(pVideoRenderer))
        return E_FAIL;
    
    return hr;
}

static HRESULT WINAPI VideoRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBasicVideo))
        *ppv = (LPVOID)&(This->IBasicVideo_vtbl);
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        *ppv = (LPVOID)&(This->IVideoWindow_vtbl);

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VideoRenderer_AddRef(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VideoRenderer_Release(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        DestroyWindow(This->hWnd);
        PostThreadMessageA(This->ThreadID, WM_QUIT, 0, 0);
        WaitForSingleObject(This->hThread, INFINITE);
        CloseHandle(This->hThread);

        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        
        if (SUCCEEDED(IPin_ConnectedTo(This->ppPins[0], &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect(This->ppPins[0]);

        IPin_Release(This->ppPins[0]);
        
        CoTaskMemFree(This->ppPins);
        This->lpVtbl = NULL;
        
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);

        TRACE("Destroying Video Renderer\n");
        CoTaskMemFree(This);
        
        return 0;
    }
    else
        return refCount;
}

/** IPersist methods **/

static HRESULT WINAPI VideoRenderer_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = CLSID_VideoRenderer;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI VideoRenderer_Stop(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_Pause(IBaseFilter * iface)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    
    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        This->rtStreamStart = tStart;
        This->state = State_Running;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);
    
    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI VideoRenderer_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    epd.cPins = 1; /* input pin */
    epd.ppPins = This->ppPins;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI VideoRenderer_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    FIXME("VideoRenderer::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI VideoRenderer_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    
    return S_OK;
}

static HRESULT WINAPI VideoRenderer_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;

    TRACE("(%p/%p)->(%p, %s)\n", This, iface, pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            strcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI VideoRenderer_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    VideoRendererImpl *This = (VideoRendererImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl VideoRenderer_Vtbl =
{
    VideoRenderer_QueryInterface,
    VideoRenderer_AddRef,
    VideoRenderer_Release,
    VideoRenderer_GetClassID,
    VideoRenderer_Stop,
    VideoRenderer_Pause,
    VideoRenderer_Run,
    VideoRenderer_GetState,
    VideoRenderer_SetSyncSource,
    VideoRenderer_GetSyncSource,
    VideoRenderer_EnumPins,
    VideoRenderer_FindPin,
    VideoRenderer_QueryFilterInfo,
    VideoRenderer_JoinFilterGraph,
    VideoRenderer_QueryVendorInfo
};

static HRESULT WINAPI VideoRenderer_InputPin_EndOfStream(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    IMediaEventSink* pEventSink;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    hr = IFilterGraph_QueryInterface(((VideoRendererImpl*)This->pin.pinInfo.pFilter)->filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
    if (SUCCEEDED(hr))
    {
        hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, 0);
        IMediaEventSink_Release(pEventSink);
    }

    return hr;
}

static const IPinVtbl VideoRenderer_InputPin_Vtbl = 
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    InputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    VideoRenderer_InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VideoRenderer_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release((IBaseFilter*)This);
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
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pAvgTimePerFrame);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitRate(IBasicVideo *iface,
					     long *pBitRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_BitErrorRate(IBasicVideo *iface,
						  long *pBitErrorRate) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBitErrorRate);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoWidth(IBasicVideo *iface,
						long *pVideoWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    *pVideoWidth = This->VideoWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_VideoHeight(IBasicVideo *iface,
						 long *pVideoHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    *pVideoHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceLeft(IBasicVideo *iface,
						long SourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceLeft);

    This->SourceRect.left = SourceLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceLeft(IBasicVideo *iface,
						long *pSourceLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    *pSourceLeft = This->SourceRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceWidth(IBasicVideo *iface,
						 long SourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceWidth);

    This->SourceRect.right = This->SourceRect.left + SourceWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceWidth(IBasicVideo *iface,
						 long *pSourceWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    *pSourceWidth = This->SourceRect.right - This->SourceRect.left;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceTop(IBasicVideo *iface,
					       long SourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceTop);

    This->SourceRect.top = SourceTop;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceTop(IBasicVideo *iface,
					       long *pSourceTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    *pSourceTop = This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_SourceHeight(IBasicVideo *iface,
						  long SourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, SourceHeight);

    This->SourceRect.bottom = This->SourceRect.top + SourceHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_SourceHeight(IBasicVideo *iface,
						  long *pSourceHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    *pSourceHeight = This->SourceRect.bottom - This->SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationLeft(IBasicVideo *iface,
						     long DestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationLeft);

    This->DestRect.left = DestinationLeft;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationLeft(IBasicVideo *iface,
						     long *pDestinationLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    *pDestinationLeft = This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationWidth(IBasicVideo *iface,
						      long DestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationWidth);

    This->DestRect.right = This->DestRect.left + DestinationWidth;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationWidth(IBasicVideo *iface,
						      long *pDestinationWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    *pDestinationWidth = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationTop(IBasicVideo *iface,
						    long DestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationTop);

    This->DestRect.top = DestinationTop;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationTop(IBasicVideo *iface,
						    long *pDestinationTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    *pDestinationTop = This->DestRect.top;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_put_DestinationHeight(IBasicVideo *iface,
						       long DestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, DestinationHeight);

    This->DestRect.right = This->DestRect.left + DestinationHeight;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_get_DestinationHeight(IBasicVideo *iface,
						       long *pDestinationHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    *pDestinationHeight = This->DestRect.right - This->DestRect.left;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_SetSourcePosition(IBasicVideo *iface,
						   long Left,
						   long Top,
						   long Width,
						   long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    This->SourceRect.left = Left;
    This->SourceRect.top = Top;
    This->SourceRect.right = Left + Width;
    This->SourceRect.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetSourcePosition(IBasicVideo *iface,
						   long *pLeft,
						   long *pTop,
						   long *pWidth,
						   long *pHeight) {
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
							long Left,
							long Top,
							long Width,
							long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    This->DestRect.left = Left;
    This->DestRect.top = Top;
    This->DestRect.right = Left + Width;
    This->DestRect.bottom = Top + Height;

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetDestinationPosition(IBasicVideo *iface,
							long *pLeft,
							long *pTop,
							long *pWidth,
							long *pHeight) {
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

    if (!GetClientRect(This->hWnd, &rect))
        return E_FAIL;
    
    This->SourceRect.left = 0;
    This->SourceRect.top = 0;
    This->SourceRect.right = rect.right;
    This->SourceRect.bottom = rect.bottom;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoSize(IBasicVideo *iface,
					      long *pWidth,
					      long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;
    
    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetVideoPaletteEntries(IBasicVideo *iface,
							long StartIndex,
							long Entries,
							long *pRetrieved,
							long *pPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%ld, %ld, %p, %p): stub !!!\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    return S_OK;
}

static HRESULT WINAPI Basicvideo_GetCurrentImage(IBasicVideo *iface,
						 long *pBufferSize,
						 long *pDIBImage) {
    ICOM_THIS_MULTI(VideoRendererImpl, IBasicVideo_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pBufferSize, pDIBImage);

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

    return VideoRenderer_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VideoRenderer_Release((IBaseFilter*)This);
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

    if (!SetWindowTextW(This->hWnd, strCaption))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Caption(IVideoWindow *iface,
					      BSTR *strCaption) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    GetWindowTextW(This->hWnd, (LPWSTR)strCaption, 100);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyle(IVideoWindow *iface,
						  long WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    LONG old;

    old = GetWindowLongA(This->hWnd, GWL_STYLE);
    
    TRACE("(%p/%p)->(%x -> %lx)\n", This, iface, old, WindowStyle);

    if (WindowStyle & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;
    
    SetWindowLongA(This->hWnd, GWL_STYLE, WindowStyle);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyle(IVideoWindow *iface,
						  long *WindowStyle) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    *WindowStyle = GetWindowLongA(This->hWnd, GWL_STYLE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowStyleEx(IVideoWindow *iface,
						    long WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, WindowStyleEx);

    if (WindowStyleEx & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;

    if (!SetWindowLongA(This->hWnd, GWL_EXSTYLE, WindowStyleEx))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowStyleEx(IVideoWindow *iface,
						    long *WindowStyleEx) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    *WindowStyleEx = GetWindowLongA(This->hWnd, GWL_EXSTYLE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_AutoShow(IVideoWindow *iface,
					       long AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, AutoShow);

    This->AutoShow = 1; /* FXIME: Should be AutoShow */;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_AutoShow(IVideoWindow *iface,
					       long *AutoShow) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    *AutoShow = This->AutoShow;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_WindowState(IVideoWindow *iface,
						  long WindowState) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_WindowState(IVideoWindow *iface,
						  long *WindowState) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, WindowState);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BackgroundPalette(IVideoWindow *iface,
							long BackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_BackgroundPalette(IVideoWindow *iface,
							long *pBackgroundPalette) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Visible(IVideoWindow *iface,
					      long Visible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Visible);

    ShowWindow(This->hWnd, Visible ? SW_SHOW : SW_HIDE);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Visible(IVideoWindow *iface,
					      long *pVisible) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    *pVisible = IsWindowVisible(This->hWnd);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Left(IVideoWindow *iface,
					   long Left) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Left);

    if (!SetWindowPos(This->hWnd, NULL, Left, This->WindowPos.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.left = Left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Left(IVideoWindow *iface,
					   long *pLeft) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    *pLeft = This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Width(IVideoWindow *iface,
					    long Width) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Width);

    if (!SetWindowPos(This->hWnd, NULL, 0, 0, Width, This->WindowPos.bottom-This->WindowPos.top, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.right = This->WindowPos.left + Width;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Width(IVideoWindow *iface,
					    long *pWidth) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    *pWidth = This->WindowPos.right - This->WindowPos.left;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Top(IVideoWindow *iface,
					  long Top) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Top);

    if (!SetWindowPos(This->hWnd, NULL, This->WindowPos.left, Top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    This->WindowPos.top = Top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Top(IVideoWindow *iface,
					  long *pTop) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    *pTop = This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Height(IVideoWindow *iface,
					     long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%ld)\n", This, iface, Height);

    if (!SetWindowPos(This->hWnd, NULL, 0, 0, This->WindowPos.right-This->WindowPos.left, Height, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->WindowPos.bottom = This->WindowPos.top + Height;

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Height(IVideoWindow *iface,
					     long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_Owner(IVideoWindow *iface,
					    OAHWND Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    SetParent(This->hWnd, (HWND)Owner);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_Owner(IVideoWindow *iface,
					    OAHWND *Owner) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    *(HWND*)Owner = GetParent(This->hWnd);

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
						  long *Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_BorderColor(IVideoWindow *iface,
						  long Color) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, Color);

    return S_OK;
}

static HRESULT WINAPI Videowindow_get_FullScreenMode(IVideoWindow *iface,
						     long *FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_put_FullScreenMode(IVideoWindow *iface,
						     long FullScreenMode) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, FullScreenMode);

    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowForeground(IVideoWindow *iface,
						      long Focus) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    BOOL ret;
    IPin* pPin;
    HRESULT hr;

    TRACE("(%p/%p)->(%ld)\n", This, iface, Focus);

    if ((Focus != FALSE) && (Focus != TRUE))
        return E_INVALIDARG;

    hr = IPin_ConnectedTo(This->ppPins[0], &pPin);
    if ((hr != S_OK) || !pPin)
        return VFW_E_NOT_CONNECTED;

    if (Focus)
        ret = SetForegroundWindow(This->hWnd);
    else
        ret = SetWindowPos(This->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

    if (!ret)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI Videowindow_NotifyOwnerMessage(IVideoWindow *iface,
						     OAHWND hwnd,
						     long uMsg,
						     LONG_PTR wParam,
						     LONG_PTR lParam) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%08x, %ld, %08lx, %08lx)\n", This, iface, (DWORD) hwnd, uMsg, wParam, lParam);

    if (!PostMessageA(This->hWnd, uMsg, wParam, lParam))
        return E_FAIL;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_SetWindowPosition(IVideoWindow *iface,
						    long Left,
						    long Top,
						    long Width,
						    long Height) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);
    
    TRACE("(%p/%p)->(%ld, %ld, %ld, %ld)\n", This, iface, Left, Top, Width, Height);

    if (!SetWindowPos(This->hWnd, NULL, Left, Top, Width, Height, SWP_NOZORDER))
        return E_FAIL;

    This->WindowPos.left = Left;
    This->WindowPos.top = Top;
    This->WindowPos.right = Left + Width;
    This->WindowPos.bottom = Top + Height;
    
    return S_OK;
}

static HRESULT WINAPI Videowindow_GetWindowPosition(IVideoWindow *iface,
						    long *pLeft,
						    long *pTop,
						    long *pWidth,
						    long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    *pLeft = This->WindowPos.left;
    *pTop = This->WindowPos.top;
    *pWidth = This->WindowPos.right - This->WindowPos.left;
    *pHeight = This->WindowPos.bottom - This->WindowPos.top;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMinIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetMaxIdealImageSize(IVideoWindow *iface,
						       long *pWidth,
						       long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p): semi stub !!!\n", This, iface, pWidth, pHeight);

    *pWidth = This->VideoWidth;
    *pHeight = This->VideoHeight;

    return S_OK;
}

static HRESULT WINAPI Videowindow_GetRestorePosition(IVideoWindow *iface,
						     long *pLeft,
						     long *pTop,
						     long *pWidth,
						     long *pHeight) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

static HRESULT WINAPI Videowindow_HideCursor(IVideoWindow *iface,
					     long HideCursor) {
    ICOM_THIS_MULTI(VideoRendererImpl, IVideoWindow_vtbl, iface);

    FIXME("(%p/%p)->(%ld): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

static HRESULT WINAPI Videowindow_IsCursorHidden(IVideoWindow *iface,
						 long *CursorHidden) {
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
