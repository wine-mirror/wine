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
    IUnknown IUnknown_inner;

    BITMAPINFOHEADER bmiheader;
    IUnknown * outer_unk;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
} VMR9Impl;

static inline VMR9Impl *impl_from_inner_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, VMR9Impl, IUnknown_inner);
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
    }
    else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
    {
        VIDEOINFOHEADER2 *format = (VIDEOINFOHEADER2 *)pmt->pbFormat;

        This->bmiheader = format->bmiHeader;

        TRACE("Resolution: %dx%d\n", format->bmiHeader.biWidth, format->bmiHeader.biHeight);
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

static HRESULT WINAPI VMR9Inner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    VMR9Impl *This = impl_from_inner_IUnknown(iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
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

    else if (IsEqualIID(riid, &IID_IBasicVideo))
        FIXME("No interface for IID_IBasicVideo\n");
    else if (IsEqualIID(riid, &IID_IBasicVideo2))
        FIXME("No interface for IID_IBasicVideo2\n");
    else if (IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for IID_IVideoWindow\n");
    else if (IsEqualIID(riid, &IID_IVMRWindowlessControl9))
        ;
    else if (IsEqualIID(riid, &IID_IVMRSurfaceAllocatorNotify9))
        ;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        FIXME("No interface for IID_IAMFilterMiscFlags\n");
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

    hr = BaseRenderer_Init(&pVMR9->renderer, &VMR9_Vtbl, outer_unk, &CLSID_VideoMixingRenderer9, (DWORD_PTR)(__FILE__ ": VMR9Impl.csFilter"), &BaseFuncTable);

    if (SUCCEEDED(hr))
    {
        *ppv = (LPVOID)pVMR9;
        TRACE("Created at %p\n", pVMR9);
    }
    else
    {
        BaseRendererImpl_Release(&pVMR9->renderer.filter.IBaseFilter_iface);
        CoTaskMemFree(pVMR9);
    }

    return hr;
}
