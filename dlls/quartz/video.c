/*
 * Common implementation of IBasicVideo
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

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static inline BaseControlVideo *impl_from_IBasicVideo(IBasicVideo *iface)
{
    return CONTAINING_RECORD(iface, BaseControlVideo, IBasicVideo_iface);
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

static const VIDEOINFOHEADER *get_video_format(BaseControlVideo *video)
{
    /* Members of VIDEOINFOHEADER up to bmiHeader are identical to those of
     * VIDEOINFOHEADER2. */
    return (const VIDEOINFOHEADER *)video->pPin->mt.pbFormat;
}

static const BITMAPINFOHEADER *get_bitmap_header(BaseControlVideo *video)
{
    const AM_MEDIA_TYPE *mt = &video->pPin->mt;
    if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
        return &((VIDEOINFOHEADER *)mt->pbFormat)->bmiHeader;
    else
        return &((VIDEOINFOHEADER2 *)mt->pbFormat)->bmiHeader;
}

static HRESULT WINAPI basic_video_get_AvgTimePerFrame(IBasicVideo *iface, REFTIME *pAvgTimePerFrame)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    if (!pAvgTimePerFrame)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    *pAvgTimePerFrame = get_video_format(This)->AvgTimePerFrame;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_BitRate(IBasicVideo *iface, LONG *pBitRate)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    if (!pBitRate)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    *pBitRate = get_video_format(This)->dwBitRate;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_BitErrorRate(IBasicVideo *iface, LONG *pBitErrorRate)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    if (!pBitErrorRate)
        return E_POINTER;
    if (!This->pPin->peer)
        return VFW_E_NOT_CONNECTED;

    *pBitErrorRate = get_video_format(This)->dwBitErrorRate;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_VideoWidth(IBasicVideo *iface, LONG *pVideoWidth)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);
    if (!pVideoWidth)
        return E_POINTER;

    *pVideoWidth = get_bitmap_header(This)->biWidth;

    return S_OK;
}

static HRESULT WINAPI basic_video_get_VideoHeight(IBasicVideo *iface, LONG *pVideoHeight)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);
    if (!pVideoHeight)
        return E_POINTER;

    *pVideoHeight = abs(get_bitmap_header(This)->biHeight);

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceLeft(IBasicVideo *iface, LONG left)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, left %d.\n", iface, left);

    if (left < 0 || This->src.right + left - This->src.left > get_bitmap_header(This)->biWidth)
        return E_INVALIDARG;

    OffsetRect(&This->src, left - This->src.left, 0);
    return S_OK;
}

static HRESULT WINAPI basic_video_get_SourceLeft(IBasicVideo *iface, LONG *pSourceLeft)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);
    if (!pSourceLeft)
        return E_POINTER;
    *pSourceLeft = This->src.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceWidth(IBasicVideo *iface, LONG width)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, width %d.\n", iface, width);

    if (width <= 0 || This->src.left + width > get_bitmap_header(This)->biWidth)
        return E_INVALIDARG;

    This->src.right = This->src.left + width;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_SourceWidth(IBasicVideo *iface, LONG *pSourceWidth)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);
    if (!pSourceWidth)
        return E_POINTER;
    *pSourceWidth = This->src.right - This->src.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceTop(IBasicVideo *iface, LONG top)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, top %d.\n", iface, top);

    if (top < 0 || This->src.bottom + top - This->src.top > get_bitmap_header(This)->biHeight)
        return E_INVALIDARG;

    OffsetRect(&This->src, 0, top - This->src.top);
    return S_OK;
}

static HRESULT WINAPI basic_video_get_SourceTop(IBasicVideo *iface, LONG *pSourceTop)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);
    if (!pSourceTop)
        return E_POINTER;
    *pSourceTop = This->src.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_SourceHeight(IBasicVideo *iface, LONG height)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, height %d.\n", iface, height);

    if (height <= 0 || This->src.top + height > get_bitmap_header(This)->biHeight)
        return E_INVALIDARG;

    This->src.bottom = This->src.top + height;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_SourceHeight(IBasicVideo *iface, LONG *pSourceHeight)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);
    if (!pSourceHeight)
        return E_POINTER;

    *pSourceHeight = This->src.bottom - This->src.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationLeft(IBasicVideo *iface, LONG left)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, left %d.\n", iface, left);

    OffsetRect(&This->dst, left - This->dst.left, 0);
    return S_OK;
}

static HRESULT WINAPI basic_video_get_DestinationLeft(IBasicVideo *iface, LONG *pDestinationLeft)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);
    if (!pDestinationLeft)
        return E_POINTER;
    *pDestinationLeft = This->dst.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationWidth(IBasicVideo *iface, LONG width)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, width %d.\n", iface, width);

    if (width <= 0)
        return E_INVALIDARG;

    This->dst.right = This->dst.left + width;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_DestinationWidth(IBasicVideo *iface, LONG *pDestinationWidth)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);
    if (!pDestinationWidth)
        return E_POINTER;
    *pDestinationWidth = This->dst.right - This->dst.left;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationTop(IBasicVideo *iface, LONG top)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, top %d.\n", iface, top);

    OffsetRect(&This->dst, 0, top - This->dst.top);
    return S_OK;
}

static HRESULT WINAPI basic_video_get_DestinationTop(IBasicVideo *iface, LONG *pDestinationTop)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);
    if (!pDestinationTop)
        return E_POINTER;
    *pDestinationTop = This->dst.top;

    return S_OK;
}

static HRESULT WINAPI basic_video_put_DestinationHeight(IBasicVideo *iface, LONG height)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("iface %p, height %d.\n", iface, height);

    if (height <= 0)
        return E_INVALIDARG;

    This->dst.bottom = This->dst.top + height;
    return S_OK;
}

static HRESULT WINAPI basic_video_get_DestinationHeight(IBasicVideo *iface, LONG *pDestinationHeight)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);
    if (!pDestinationHeight)
        return E_POINTER;
    *pDestinationHeight = This->dst.bottom - This->dst.top;

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
    This->src = SourceRect;
    return S_OK;
}

static HRESULT WINAPI basic_video_GetSourcePosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    if (!pLeft || !pTop || !pWidth || !pHeight)
        return E_POINTER;

    *pLeft = This->src.left;
    *pTop = This->src.top;
    *pWidth = This->src.right - This->src.left;
    *pHeight = This->src.bottom - This->src.top;

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
    This->dst = DestRect;
    return S_OK;
}

static HRESULT WINAPI basic_video_GetDestinationPosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlVideo *This = impl_from_IBasicVideo(iface);

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    if (!pLeft || !pTop || !pWidth || !pHeight)
        return E_POINTER;

    *pLeft = This->dst.left;
    *pTop = This->dst.top;
    *pWidth = This->dst.right - This->dst.left;
    *pHeight = This->dst.bottom - This->dst.top;

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
    BaseControlVideo *This = impl_from_IBasicVideo(iface);
    const BITMAPINFOHEADER *bitmap_header = get_bitmap_header(This);

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    if (!pWidth || !pHeight)
        return E_POINTER;

    *pHeight = bitmap_header->biHeight;
    *pWidth = bitmap_header->biWidth;

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
    FIXME("iface %p, stub!\n", iface);

    return S_OK;
}

static HRESULT WINAPI basic_video_IsUsingDefaultDestination(IBasicVideo *iface)
{
    FIXME("iface %p, stub!\n", iface);

    return S_OK;
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

void basic_video_init(BaseControlVideo *video, struct strmbase_filter *filter,
        struct strmbase_pin *pin, const BaseControlVideoFuncTable *func_table)
{
    video->IBasicVideo_iface.lpVtbl = &basic_video_vtbl;
    video->pFilter = filter;
    video->pPin = pin;
    video->pFuncsTable = func_table;
}
