/*
 * Video Mixing Renderer for dx9
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
 * Copyright 2012 Aric Stewart
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
#include "d3d9.h"
#include "vmr9.h"
#include "pin.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct
{
    BaseRenderer renderer;
    BaseControlWindow baseControlWindow;
    BaseControlVideo baseControlVideo;

    IUnknown IUnknown_inner;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;

    BITMAPINFOHEADER bmiheader;
    IUnknown * outer_unk;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;

    RECT source_rect;
    RECT target_rect;
    LONG VideoWidth;
    LONG VideoHeight;
} VMR9Impl;

static inline VMR9Impl *impl_from_inner_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IUnknown_inner);
}

static inline VMR9Impl *impl_from_BaseWindow( BaseWindow *wnd )
{
    return CONTAINING_RECORD(wnd, VMR9Impl, baseControlWindow.baseWindow);
}

static inline VMR9Impl *impl_from_IVideoWindow( IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, baseControlWindow.IVideoWindow_iface);
}

static inline VMR9Impl *impl_from_BaseControlVideo( BaseControlVideo *cvid )
{
    return CONTAINING_RECORD(cvid, VMR9Impl, baseControlVideo);
}

static inline VMR9Impl *impl_from_IBasicVideo( IBasicVideo *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, baseControlVideo.IBasicVideo_iface);
}

static inline VMR9Impl *impl_from_IAMFilterMiscFlags( IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IAMFilterMiscFlags_iface);
}

static HRESULT WINAPI VMR9_DoRenderSample(BaseRenderer *iface, IMediaSample * pSample)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LPBYTE pbSrcStream = NULL;
    REFERENCE_TIME tStart, tStop;
    VMR9PresentationInfo info;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        info.dwFlags = VMR9Sample_SrcDstRectsValid;
    else
        info.dwFlags = VMR9Sample_SrcDstRectsValid | VMR9Sample_TimeValid;

    if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_Discontinuity;

    if (IMediaSample_IsPreroll(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_Preroll;

    if (IMediaSample_IsSyncPoint(pSample) == S_OK)
        info.dwFlags |= VMR9Sample_SyncPoint;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    info.rtStart = tStart;
    info.rtEnd = tStop;
    info.szAspectRatio.cx = This->bmiheader.biWidth;
    info.szAspectRatio.cy = This->bmiheader.biHeight;

    return hr;
}

static HRESULT WINAPI VMR9_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    VMR9Impl *This = (VMR9Impl*)iface;

    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Video) || !pmt->pbFormat)
        return S_FALSE;

    /* Ignore subtype, test for bicompression instead */
    if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
    {
        VIDEOINFOHEADER *format = (VIDEOINFOHEADER *)pmt->pbFormat;

        This->bmiheader = format->bmiHeader;
        TRACE("Resolution: %dx%d\n", format->bmiHeader.biWidth, format->bmiHeader.biHeight);
        This->source_rect.right = This->VideoWidth = format->bmiHeader.biWidth;
        This->source_rect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        This->source_rect.top = This->source_rect.left = 0;
    }
    else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
    {
        VIDEOINFOHEADER2 *format = (VIDEOINFOHEADER2 *)pmt->pbFormat;

        This->bmiheader = format->bmiHeader;

        TRACE("Resolution: %dx%d\n", format->bmiHeader.biWidth, format->bmiHeader.biHeight);
        This->source_rect.right = This->VideoWidth = format->bmiHeader.biWidth;
        This->source_rect.bottom = This->VideoHeight = format->bmiHeader.biHeight;
        This->source_rect.top = This->source_rect.left = 0;
    }
    else
    {
        ERR("Format type %s not supported\n", debugstr_guid(&pmt->formattype));
        return S_FALSE;
    }
    if (This->bmiheader.biCompression)
        return S_FALSE;
    return S_OK;
}

HRESULT WINAPI VMR9_ShouldDrawSampleNow(BaseRenderer *This, IMediaSample *pSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime)
{
    /* Preroll means the sample isn't shown, this is used for key frames and things like that */
    if (IMediaSample_IsPreroll(pSample) == S_OK)
        return E_FAIL;
    return S_FALSE;
}

static const BaseRendererFuncTable BaseFuncTable = {
    VMR9_CheckMediaType,
    VMR9_DoRenderSample,
    /**/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    VMR9_ShouldDrawSampleNow,
    NULL,
    /**/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static LPWSTR WINAPI VMR9_GetClassWindowStyles(BaseWindow *This, DWORD *pClassStyles, DWORD *pWindowStyles, DWORD *pWindowStylesEx)
{
    static WCHAR classnameW[] = { 'I','V','M','R','9',' ','C','l','a','s','s', 0 };

    *pClassStyles = 0;
    *pWindowStyles = WS_SIZEBOX;
    *pWindowStylesEx = 0;

    return classnameW;
}

static RECT WINAPI VMR9_GetDefaultRect(BaseWindow *This)
{
    VMR9Impl* pVMR9 = impl_from_BaseWindow(This);
    static RECT defRect;

    defRect.left = defRect.top = 0;
    defRect.right = pVMR9->VideoWidth;
    defRect.bottom = pVMR9->VideoHeight;

    return defRect;
}

static BOOL WINAPI VMR9_OnSize(BaseWindow *This, LONG Width, LONG Height)
{
    VMR9Impl* pVMR9 = impl_from_BaseWindow(This);

    TRACE("WM_SIZE %d %d\n", Width, Height);
    GetClientRect(This->hWnd, &pVMR9->target_rect);
    TRACE("WM_SIZING: DestRect=(%d,%d),(%d,%d)\n",
        pVMR9->target_rect.left,
        pVMR9->target_rect.top,
        pVMR9->target_rect.right - pVMR9->target_rect.left,
        pVMR9->target_rect.bottom - pVMR9->target_rect.top);
    return BaseWindowImpl_OnSize(This, Width, Height);
}

static const BaseWindowFuncTable renderer_BaseWindowFuncTable = {
    VMR9_GetClassWindowStyles,
    VMR9_GetDefaultRect,
    NULL,
    BaseControlWindowImpl_PossiblyEatMessage,
    VMR9_OnSize,
};

HRESULT WINAPI VMR9_GetSourceRect(BaseControlVideo* This, RECT *pSourceRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(pSourceRect,&pVMR9->source_rect);
    return S_OK;
}

HRESULT WINAPI VMR9_GetStaticImage(BaseControlVideo* This, LONG *pBufferSize, LONG *pDIBImage)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    BITMAPINFOHEADER *bmiHeader;
    LONG needed_size;
    AM_MEDIA_TYPE *amt = &pVMR9->renderer.pInputPin->pin.mtCurrent;
    char *ptr;

    FIXME("(%p/%p)->(%p, %p): partial stub\n", pVMR9, This, pBufferSize, pDIBImage);

    EnterCriticalSection(&pVMR9->renderer.filter.csFilter);

    if (!pVMR9->renderer.pMediaSample)
    {
         LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
         return (pVMR9->renderer.filter.state == State_Paused ? E_UNEXPECTED : VFW_E_NOT_PAUSED);
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
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return VFW_E_RUNTIME_ERROR;
    }

    needed_size = bmiHeader->biSize;
    needed_size += IMediaSample_GetActualDataLength(pVMR9->renderer.pMediaSample);

    if (!pDIBImage)
    {
        *pBufferSize = needed_size;
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return S_OK;
    }

    if (needed_size < *pBufferSize)
    {
        ERR("Buffer too small %u/%u\n", needed_size, *pBufferSize);
        LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
        return E_FAIL;
    }
    *pBufferSize = needed_size;

    memcpy(pDIBImage, bmiHeader, bmiHeader->biSize);
    IMediaSample_GetPointer(pVMR9->renderer.pMediaSample, (BYTE **)&ptr);
    memcpy((char *)pDIBImage + bmiHeader->biSize, ptr, IMediaSample_GetActualDataLength(pVMR9->renderer.pMediaSample));

    LeaveCriticalSection(&pVMR9->renderer.filter.csFilter);
    return S_OK;
}

HRESULT WINAPI VMR9_GetTargetRect(BaseControlVideo* This, RECT *pTargetRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(pTargetRect,&pVMR9->target_rect);
    return S_OK;
}

VIDEOINFOHEADER* WINAPI VMR9_GetVideoFormat(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    AM_MEDIA_TYPE *pmt;

    TRACE("(%p/%p)\n", pVMR9, This);

    pmt = &pVMR9->renderer.pInputPin->pin.mtCurrent;
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

HRESULT WINAPI VMR9_IsDefaultSourceRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    FIXME("(%p/%p)->(): stub !!!\n", pVMR9, This);

    return S_OK;
}

HRESULT WINAPI VMR9_IsDefaultTargetRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    FIXME("(%p/%p)->(): stub !!!\n", pVMR9, This);

    return S_OK;
}

HRESULT WINAPI VMR9_SetDefaultSourceRect(BaseControlVideo* This)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);

    pVMR9->source_rect.left = 0;
    pVMR9->source_rect.top = 0;
    pVMR9->source_rect.right = pVMR9->VideoWidth;
    pVMR9->source_rect.bottom = pVMR9->VideoHeight;

    return S_OK;
}

HRESULT WINAPI VMR9_SetDefaultTargetRect(BaseControlVideo* This)
{
    RECT rect;
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);

    if (!GetClientRect(pVMR9->baseControlWindow.baseWindow.hWnd, &rect))
        return E_FAIL;

    pVMR9->target_rect.left = 0;
    pVMR9->target_rect.top = 0;
    pVMR9->target_rect.right = rect.right;
    pVMR9->target_rect.bottom = rect.bottom;

    return S_OK;
}

HRESULT WINAPI VMR9_SetSourceRect(BaseControlVideo* This, RECT *pSourceRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(&pVMR9->source_rect,pSourceRect);
    return S_OK;
}

HRESULT WINAPI VMR9_SetTargetRect(BaseControlVideo* This, RECT *pTargetRect)
{
    VMR9Impl* pVMR9 = impl_from_BaseControlVideo(This);
    CopyRect(&pVMR9->target_rect,pTargetRect);
    return S_OK;
}

static const BaseControlVideoFuncTable renderer_BaseControlVideoFuncTable = {
    VMR9_GetSourceRect,
    VMR9_GetStaticImage,
    VMR9_GetTargetRect,
    VMR9_GetVideoFormat,
    VMR9_IsDefaultSourceRect,
    VMR9_IsDefaultTargetRect,
    VMR9_SetDefaultSourceRect,
    VMR9_SetDefaultTargetRect,
    VMR9_SetSourceRect,
    VMR9_SetTargetRect
};

static HRESULT WINAPI VMR9Inner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        *ppv = &This->baseControlWindow.IVideoWindow_iface;
    else if (IsEqualIID(riid, &IID_IBasicVideo))
        *ppv = &This->baseControlVideo.IBasicVideo_iface;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_iface;
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

    else if (IsEqualIID(riid, &IID_IBasicVideo2))
        FIXME("No interface for IID_IBasicVideo2\n");
    else if (IsEqualIID(riid, &IID_IVMRWindowlessControl9))
        ;
    else if (IsEqualIID(riid, &IID_IVMRSurfaceAllocatorNotify9))
        ;
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        FIXME("No interface for IID_IMediaPosition\n");
    else if (IsEqualIID(riid, &IID_IQualProp))
        FIXME("No interface for IID_IQualProp\n");
    else if (IsEqualIID(riid, &IID_IVMRAspectRatioControl9))
        FIXME("No interface for IID_IVMRAspectRatioControl9\n");
    else if (IsEqualIID(riid, &IID_IVMRDeinterlaceControl9))
        FIXME("No interface for IID_IVMRDeinterlaceControl9\n");
    else if (IsEqualIID(riid, &IID_IVMRMixerBitmap9))
        FIXME("No interface for IID_IVMRMixerBitmap9\n");
    else if (IsEqualIID(riid, &IID_IVMRMonitorConfig9))
        FIXME("No interface for IID_IVMRMonitorConfig9\n");
    else if (IsEqualIID(riid, &IID_IVMRMixerControl9))
        FIXME("No interface for IID_IVMRMixerControl9\n");
    else
        FIXME("No interface for %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI VMR9Inner_AddRef(IUnknown * iface)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    ULONG refCount = BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI VMR9Inner_Release(IUnknown * iface)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    ULONG refCount = BaseRendererImpl_Release(&This->renderer.filter.IBaseFilter_iface);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");

        CoTaskMemFree(This);
    }
    return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    VMR9Inner_QueryInterface,
    VMR9Inner_AddRef,
    VMR9Inner_Release
};

static HRESULT WINAPI VMR9_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = (VMR9Impl *)iface;

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->outer_unk)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->outer_unk, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef(&This->IUnknown_inner);
            hr = IUnknown_QueryInterface(&This->IUnknown_inner, riid, ppv);
            IUnknown_Release(&This->IUnknown_inner);
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface(&This->IUnknown_inner, riid, ppv);
}

static ULONG WINAPI VMR9_AddRef(IBaseFilter * iface)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LONG ret;

    if (This->outer_unk && This->bUnkOuterValid)
        ret = IUnknown_AddRef(This->outer_unk);
    else
        ret = IUnknown_AddRef(&This->IUnknown_inner);

    TRACE("(%p)->AddRef from %d\n", iface, ret - 1);

    return ret;
}

static ULONG WINAPI VMR9_Release(IBaseFilter * iface)
{
    VMR9Impl *This = (VMR9Impl *)iface;
    LONG ret;

    if (This->outer_unk && This->bUnkOuterValid)
        ret = IUnknown_Release(This->outer_unk);
    else
        ret = IUnknown_Release(&This->IUnknown_inner);

    TRACE("(%p)->Release from %d\n", iface, ret + 1);

    if (ret)
        return ret;
    return 0;
}

static const IBaseFilterVtbl VMR9_Vtbl =
{
    VMR9_QueryInterface,
    VMR9_AddRef,
    VMR9_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    BaseRendererImpl_Pause,
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
static HRESULT WINAPI Videowindow_QueryInterface(IVideoWindow *iface, REFIID riid, LPVOID*ppvObj)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Videowindow_AddRef(IVideoWindow *iface)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Videowindow_Release(IVideoWindow *iface)
{
    VMR9Impl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    Videowindow_QueryInterface,
    Videowindow_AddRef,
    Videowindow_Release,
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
    BaseControlWindowImpl_get_FullScreenMode,
    BaseControlWindowImpl_put_FullScreenMode,
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

/*** IUnknown methods ***/
static HRESULT WINAPI Basicvideo_QueryInterface(IBasicVideo *iface, REFIID riid, LPVOID * ppvObj)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Basicvideo_AddRef(IBasicVideo *iface)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Basicvideo_Release(IBasicVideo *iface)
{
    VMR9Impl *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
}

static const IBasicVideoVtbl IBasicVideo_VTable =
{
    Basicvideo_QueryInterface,
    Basicvideo_AddRef,
    Basicvideo_Release,
    BaseControlVideoImpl_GetTypeInfoCount,
    BaseControlVideoImpl_GetTypeInfo,
    BaseControlVideoImpl_GetIDsOfNames,
    BaseControlVideoImpl_Invoke,
    BaseControlVideoImpl_get_AvgTimePerFrame,
    BaseControlVideoImpl_get_BitRate,
    BaseControlVideoImpl_get_BitErrorRate,
    BaseControlVideoImpl_get_VideoWidth,
    BaseControlVideoImpl_get_VideoHeight,
    BaseControlVideoImpl_put_SourceLeft,
    BaseControlVideoImpl_get_SourceLeft,
    BaseControlVideoImpl_put_SourceWidth,
    BaseControlVideoImpl_get_SourceWidth,
    BaseControlVideoImpl_put_SourceTop,
    BaseControlVideoImpl_get_SourceTop,
    BaseControlVideoImpl_put_SourceHeight,
    BaseControlVideoImpl_get_SourceHeight,
    BaseControlVideoImpl_put_DestinationLeft,
    BaseControlVideoImpl_get_DestinationLeft,
    BaseControlVideoImpl_put_DestinationWidth,
    BaseControlVideoImpl_get_DestinationWidth,
    BaseControlVideoImpl_put_DestinationTop,
    BaseControlVideoImpl_get_DestinationTop,
    BaseControlVideoImpl_put_DestinationHeight,
    BaseControlVideoImpl_get_DestinationHeight,
    BaseControlVideoImpl_SetSourcePosition,
    BaseControlVideoImpl_GetSourcePosition,
    BaseControlVideoImpl_SetDefaultSourcePosition,
    BaseControlVideoImpl_SetDestinationPosition,
    BaseControlVideoImpl_GetDestinationPosition,
    BaseControlVideoImpl_SetDefaultDestinationPosition,
    BaseControlVideoImpl_GetVideoSize,
    BaseControlVideoImpl_GetVideoPaletteEntries,
    BaseControlVideoImpl_GetCurrentImage,
    BaseControlVideoImpl_IsUsingDefaultSource,
    BaseControlVideoImpl_IsUsingDefaultDestination
};

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    VMR9Impl *This = impl_from_IAMFilterMiscFlags(iface);
    return VMR9_Release(&This->renderer.filter.IBaseFilter_iface);
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

HRESULT VMR9Impl_create(IUnknown * outer_unk, LPVOID * ppv)
{
    HRESULT hr;
    VMR9Impl * pVMR9;

    TRACE("(%p, %p)\n", outer_unk, ppv);

    *ppv = NULL;

    pVMR9 = CoTaskMemAlloc(sizeof(VMR9Impl));

    pVMR9->outer_unk = outer_unk;
    pVMR9->bUnkOuterValid = FALSE;
    pVMR9->bAggregatable = FALSE;
    pVMR9->IUnknown_inner.lpVtbl = &IInner_VTable;
    pVMR9->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_Vtbl;

    hr = BaseRenderer_Init(&pVMR9->renderer, &VMR9_Vtbl, outer_unk, &CLSID_VideoMixingRenderer9, (DWORD_PTR)(__FILE__ ": VMR9Impl.csFilter"), &BaseFuncTable);
    if (FAILED(hr))
        goto fail;

    hr = BaseControlWindow_Init(&pVMR9->baseControlWindow, &IVideoWindow_VTable, &pVMR9->renderer.filter, &pVMR9->renderer.filter.csFilter, &pVMR9->renderer.pInputPin->pin, &renderer_BaseWindowFuncTable);
    if (FAILED(hr))
        goto fail;

    hr = BaseControlVideo_Init(&pVMR9->baseControlVideo, &IBasicVideo_VTable, &pVMR9->renderer.filter, &pVMR9->renderer.filter.csFilter, &pVMR9->renderer.pInputPin->pin, &renderer_BaseControlVideoFuncTable);
    if (FAILED(hr))
        goto fail;

    *ppv = (LPVOID)pVMR9;
    ZeroMemory(&pVMR9->source_rect, sizeof(RECT));
    ZeroMemory(&pVMR9->target_rect, sizeof(RECT));
    TRACE("Created at %p\n", pVMR9);
    return hr;

fail:
    BaseRendererImpl_Release(&pVMR9->renderer.filter.IBaseFilter_iface);
    CoTaskMemFree(pVMR9);
    return hr;
}
