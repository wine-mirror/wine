/*
 * Generic Implementation of strmbase video classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static inline BaseControlVideo *impl_from_IBasicVideo(IBasicVideo *iface)
{
    return CONTAINING_RECORD(iface, BaseControlVideo, IBasicVideo_iface);
}

HRESULT WINAPI BaseControlVideo_Destroy(BaseControlVideo *pControlVideo)
{
    return S_OK;
}

static HRESULT BaseControlVideoImpl_CheckSourceRect(BaseControlVideo *This, RECT *pSourceRect)
{
    LONG VideoWidth, VideoHeight;
    HRESULT hr;

    if (IsRectEmpty(pSourceRect))
        return E_INVALIDARG;

    hr = IBasicVideo_GetVideoSize(&This->IBasicVideo_iface, &VideoWidth, &VideoHeight);
    if (FAILED(hr))
        return hr;

    if (pSourceRect->top < 0 || pSourceRect->left < 0 ||
        pSourceRect->bottom > VideoHeight || pSourceRect->right > VideoWidth)
        return E_INVALIDARG;

    return S_OK;
}

static HRESULT BaseControlVideoImpl_CheckTargetRect(BaseControlVideo *This, RECT *pTargetRect)
{
    if (IsRectEmpty(pTargetRect))
        return E_INVALIDARG;

    return S_OK;
}

static HRESULT WINAPI basic_video_QueryInterface(IBasicVideo *iface, REFIID iid, void **out)
{
    BaseControlVideo *video = impl_from_IBasicVideo(iface);
    return IUnknown_QueryInterface(video->pFilter->outer_unk, iid, out);
}

static ULONG WINAPI basic_video_AddRef(IBasicVideo *iface)
{
    BaseControlVideo *video = impl_from_IBasicVideo(iface);
    return IUnknown_AddRef(video->pFilter->outer_unk);
}

static ULONG WINAPI basic_video_Release(IBasicVideo *iface)
{
    BaseControlVideo *video = impl_from_IBasicVideo(iface);
    return IUnknown_Release(video->pFilter->outer_unk);
}

static HRESULT WINAPI basic_video_GetTypeInfoCount(IBasicVideo *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI basic_video_GetTypeInfo(IBasicVideo *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicVideo_tid, typeinfo);
}

static HRESULT WINAPI basic_video_GetIDsOfNames(IBasicVideo *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI basic_video_Invoke(IBasicVideo *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI basic_video_get_AvgTimePerFrame(IBasicVideo *iface, REFTIME *pAvgTimePerFrame)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    if (!pAvgTimePerFrame)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pAvgTimePerFrame = vih->AvgTimePerFrame;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_BitRate(IBasicVideo *iface, LONG *pBitRate)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    if (!pBitRate)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pBitRate = vih->dwBitRate;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_BitErrorRate(IBasicVideo *iface, LONG *pBitErrorRate)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    if (!pBitErrorRate)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pBitErrorRate = vih->dwBitErrorRate;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_VideoWidth(IBasicVideo *iface, LONG *pVideoWidth)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);
    if (!pVideoWidth)
        return E_POINTER;

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pVideoWidth = vih->bmiHeader.biWidth;

    return S_OK;
}

static HRESULT WINAPI basic_video_get_VideoHeight(IBasicVideo *iface, LONG *pVideoHeight)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);
    if (!pVideoHeight)
        return E_POINTER;

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pVideoHeight = abs(vih->bmiHeader.biHeight);

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceLeft(IBasicVideo *iface, LONG SourceLeft)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceLeft);
    hr = This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    if (SUCCEEDED(hr))
    {
        SourceRect.right = (SourceRect.right - SourceRect.left) + SourceLeft;
        SourceRect.left = SourceLeft;
        hr = BaseControlVideoImpl_CheckSourceRect(This, &SourceRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetSourceRect(This, &SourceRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_SourceLeft(IBasicVideo *iface, LONG *pSourceLeft)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);
    if (!pSourceLeft)
        return E_POINTER;
    This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    *pSourceLeft = SourceRect.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceWidth(IBasicVideo *iface, LONG SourceWidth)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceWidth);
    hr = This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    if (SUCCEEDED(hr))
    {
        SourceRect.right = SourceRect.left + SourceWidth;
        hr = BaseControlVideoImpl_CheckSourceRect(This, &SourceRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetSourceRect(This, &SourceRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_SourceWidth(IBasicVideo *iface, LONG *pSourceWidth)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);
    if (!pSourceWidth)
        return E_POINTER;
    This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    *pSourceWidth = SourceRect.right - SourceRect.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceTop(IBasicVideo *iface, LONG SourceTop)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceTop);
    hr = This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    if (SUCCEEDED(hr))
    {
        SourceRect.bottom = (SourceRect.bottom - SourceRect.top) + SourceTop;
        SourceRect.top = SourceTop;
        hr = BaseControlVideoImpl_CheckSourceRect(This, &SourceRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetSourceRect(This, &SourceRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_SourceTop(IBasicVideo *iface, LONG *pSourceTop)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);
    if (!pSourceTop)
        return E_POINTER;
    This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    *pSourceTop = SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceHeight(IBasicVideo *iface, LONG SourceHeight)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceHeight);
    hr = This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);
    if (SUCCEEDED(hr))
    {
        SourceRect.bottom = SourceRect.top + SourceHeight;
        hr = BaseControlVideoImpl_CheckSourceRect(This, &SourceRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetSourceRect(This, &SourceRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_SourceHeight(IBasicVideo *iface, LONG *pSourceHeight)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);
    if (!pSourceHeight)
        return E_POINTER;
    This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);

    *pSourceHeight = SourceRect.bottom - SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationLeft(IBasicVideo *iface, LONG DestinationLeft)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationLeft);
    hr = This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    if (SUCCEEDED(hr))
    {
        DestRect.right = (DestRect.right - DestRect.left) + DestinationLeft;
        DestRect.left = DestinationLeft;
        hr = BaseControlVideoImpl_CheckTargetRect(This, &DestRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetTargetRect(This, &DestRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_DestinationLeft(IBasicVideo *iface, LONG *pDestinationLeft)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);
    if (!pDestinationLeft)
        return E_POINTER;
    This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    *pDestinationLeft = DestRect.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationWidth(IBasicVideo *iface, LONG DestinationWidth)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationWidth);
    hr = This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    if (SUCCEEDED(hr))
    {
        DestRect.right = DestRect.left + DestinationWidth;
        hr = BaseControlVideoImpl_CheckTargetRect(This, &DestRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetTargetRect(This, &DestRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_DestinationWidth(IBasicVideo *iface, LONG *pDestinationWidth)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);
    if (!pDestinationWidth)
        return E_POINTER;
    This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    *pDestinationWidth = DestRect.right - DestRect.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationTop(IBasicVideo *iface, LONG DestinationTop)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationTop);
    hr = This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    if (SUCCEEDED(hr))
    {
        DestRect.bottom = (DestRect.bottom - DestRect.top) + DestinationTop;
        DestRect.top = DestinationTop;
        hr = BaseControlVideoImpl_CheckTargetRect(This, &DestRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetTargetRect(This, &DestRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_DestinationTop(IBasicVideo *iface, LONG *pDestinationTop)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);
    if (!pDestinationTop)
        return E_POINTER;
    This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    *pDestinationTop = DestRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationHeight(IBasicVideo *iface, LONG DestinationHeight)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationHeight);
    hr = This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    if (SUCCEEDED(hr))
    {
        DestRect.bottom = DestRect.top + DestinationHeight;
        hr = BaseControlVideoImpl_CheckTargetRect(This, &DestRect);
    }
    if (SUCCEEDED(hr))
        hr = This->pFuncsTable->pfnSetTargetRect(This, &DestRect);

    return hr;
}

static HRESULT WINAPI basic_video_get_DestinationHeight(IBasicVideo *iface, LONG *pDestinationHeight)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);
    if (!pDestinationHeight)
        return E_POINTER;
    This->pFuncsTable->pfnGetTargetRect(This, &DestRect);
    *pDestinationHeight = DestRect.bottom - DestRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_SetSourcePosition(IBasicVideo *iface, LONG Left, LONG Top, LONG Width, LONG Height)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    SetRect(&SourceRect, Left, Top, Left + Width, Top + Height);
    if (FAILED(BaseControlVideoImpl_CheckSourceRect(This, &SourceRect)))
        return E_INVALIDARG;
    return This->pFuncsTable->pfnSetSourceRect(This, &SourceRect);
}

static HRESULT WINAPI basic_video_GetSourcePosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    RECT SourceRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    if (!pLeft || !pTop || !pWidth || !pHeight)
        return E_POINTER;
    This->pFuncsTable->pfnGetSourceRect(This, &SourceRect);

    *pLeft = SourceRect.left;
    *pTop = SourceRect.top;
    *pWidth = SourceRect.right - SourceRect.left;
    *pHeight = SourceRect.bottom - SourceRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_SetDefaultSourcePosition(IBasicVideo *iface)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);
    return This->pFuncsTable->pfnSetDefaultSourceRect(This);
}

static HRESULT WINAPI basic_video_SetDestinationPosition(IBasicVideo *iface, LONG Left, LONG Top, LONG Width, LONG Height)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    SetRect(&DestRect, Left, Top, Left + Width, Top + Height);
    if (FAILED(BaseControlVideoImpl_CheckTargetRect(This, &DestRect)))
        return E_INVALIDARG;
    return This->pFuncsTable->pfnSetTargetRect(This, &DestRect);
}

static HRESULT WINAPI basic_video_GetDestinationPosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    RECT DestRect;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    if (!pLeft || !pTop || !pWidth || !pHeight)
        return E_POINTER;
    This->pFuncsTable->pfnGetTargetRect(This, &DestRect);

    *pLeft = DestRect.left;
    *pTop = DestRect.top;
    *pWidth = DestRect.right - DestRect.left;
    *pHeight = DestRect.bottom - DestRect.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_SetDefaultDestinationPosition(IBasicVideo *iface)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->()\n", This, iface);
    return This->pFuncsTable->pfnSetDefaultTargetRect(This);
}

static HRESULT WINAPI basic_video_GetVideoSize(IBasicVideo *iface, LONG *pWidth, LONG *pHeight)
{
    VIDEOINFOHEADER *vih;
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    if (!pWidth || !pHeight)
        return E_POINTER;

    vih = This->pFuncsTable->pfnGetVideoFormat(This);
    *pHeight = vih->bmiHeader.biHeight;
    *pWidth = vih->bmiHeader.biWidth;

    return S_OK;
}

static HRESULT WINAPI basic_video_GetVideoPaletteEntries(IBasicVideo *iface, LONG StartIndex, LONG Entries, LONG *pRetrieved, LONG *pPalette)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%d, %d, %p, %p)\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);
    if (!pRetrieved || !pPalette)
        return E_POINTER;

    *pRetrieved = 0;
    return VFW_E_NO_PALETTE_AVAILABLE;
}

static HRESULT WINAPI basic_video_GetCurrentImage(IBasicVideo *iface, LONG *pBufferSize, LONG *pDIBImage)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    if (!pBufferSize || !pDIBImage)
        return E_POINTER;

    return This->pFuncsTable->pfnGetStaticImage(This, pBufferSize, pDIBImage);
}

static HRESULT WINAPI basic_video_IsUsingDefaultSource(IBasicVideo *iface)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    return This->pFuncsTable->pfnIsDefaultSourceRect(This);
}

static HRESULT WINAPI basic_video_IsUsingDefaultDestination(IBasicVideo *iface)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    return This->pFuncsTable->pfnIsDefaultTargetRect(This);
}

static const IBasicVideoVtbl basic_video_vtbl =
{
    basic_video_QueryInterface,
    basic_video_AddRef,
    basic_video_Release,
    basic_video_GetTypeInfoCount,
    basic_video_GetTypeInfo,
    basic_video_GetIDsOfNames,
    basic_video_Invoke,
    basic_video_get_AvgTimePerFrame,
    basic_video_get_BitRate,
    basic_video_get_BitErrorRate,
    basic_video_get_VideoWidth,
    basic_video_get_VideoHeight,
    basic_video_put_SourceLeft,
    basic_video_get_SourceLeft,
    basic_video_put_SourceWidth,
    basic_video_get_SourceWidth,
    basic_video_put_SourceTop,
    basic_video_get_SourceTop,
    basic_video_put_SourceHeight,
    basic_video_get_SourceHeight,
    basic_video_put_DestinationLeft,
    basic_video_get_DestinationLeft,
    basic_video_put_DestinationWidth,
    basic_video_get_DestinationWidth,
    basic_video_put_DestinationTop,
    basic_video_get_DestinationTop,
    basic_video_put_DestinationHeight,
    basic_video_get_DestinationHeight,
    basic_video_SetSourcePosition,
    basic_video_GetSourcePosition,
    basic_video_SetDefaultSourcePosition,
    basic_video_SetDestinationPosition,
    basic_video_GetDestinationPosition,
    basic_video_SetDefaultDestinationPosition,
    basic_video_GetVideoSize,
    basic_video_GetVideoPaletteEntries,
    basic_video_GetCurrentImage,
    basic_video_IsUsingDefaultSource,
    basic_video_IsUsingDefaultDestination
};

HRESULT WINAPI strmbase_video_init(BaseControlVideo *video, struct strmbase_filter *filter,
        struct strmbase_pin *pin, const BaseControlVideoFuncTable *func_table)
{
    video->IBasicVideo_iface.lpVtbl = &basic_video_vtbl;
    video->pFilter = filter;
    video->pPin = pin;
    video->pFuncsTable = func_table;

    return S_OK;
}
