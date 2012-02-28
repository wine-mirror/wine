/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct BitmapScaler {
    IWICBitmapScaler IWICBitmapScaler_iface;
    LONG ref;
    IWICBitmapSource *source;
    UINT width, height;
    WICBitmapInterpolationMode mode;
    CRITICAL_SECTION lock; /* must be held when initialized */
} BitmapScaler;

static inline BitmapScaler *impl_from_IWICBitmapScaler(IWICBitmapScaler *iface)
{
    return CONTAINING_RECORD(iface, BitmapScaler, IWICBitmapScaler_iface);
}

static HRESULT WINAPI BitmapScaler_QueryInterface(IWICBitmapScaler *iface, REFIID iid,
    void **ppv)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapScaler, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapScaler_AddRef(IWICBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapScaler_Release(IWICBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->source) IWICBitmapSource_Release(This->source);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapScaler_GetSize(IWICBitmapScaler *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (!puiWidth || !puiHeight)
        return E_INVALIDARG;

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;

    *puiWidth = This->width;
    *puiHeight = This->height;

    return S_OK;
}

static HRESULT WINAPI BitmapScaler_GetPixelFormat(IWICBitmapScaler *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    if (!pPixelFormat)
        return E_INVALIDARG;

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;

    return IWICBitmapSource_GetPixelFormat(This->source, pPixelFormat);
}

static HRESULT WINAPI BitmapScaler_GetResolution(IWICBitmapScaler *iface,
    double *pDpiX, double *pDpiY)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    if (!pDpiX || !pDpiY)
        return E_INVALIDARG;

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;

    return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
}

static HRESULT WINAPI BitmapScaler_CopyPalette(IWICBitmapScaler *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapScaler_CopyPixels(IWICBitmapScaler *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p): stub\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapScaler_Initialize(IWICBitmapScaler *iface,
    IWICBitmapSource *pISource, UINT uiWidth, UINT uiHeight,
    WICBitmapInterpolationMode mode)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%u,%u,%u)\n", iface, pISource, uiWidth, uiHeight, mode);

    EnterCriticalSection(&This->lock);

    if (This->source)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    IWICBitmapSource_AddRef(pISource);
    This->source = pISource;

    This->width = uiWidth;
    This->height = uiHeight;
    This->mode = mode;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static const IWICBitmapScalerVtbl BitmapScaler_Vtbl = {
    BitmapScaler_QueryInterface,
    BitmapScaler_AddRef,
    BitmapScaler_Release,
    BitmapScaler_GetSize,
    BitmapScaler_GetPixelFormat,
    BitmapScaler_GetResolution,
    BitmapScaler_CopyPalette,
    BitmapScaler_CopyPixels,
    BitmapScaler_Initialize
};

HRESULT BitmapScaler_Create(IWICBitmapScaler **scaler)
{
    BitmapScaler *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapScaler));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapScaler_iface.lpVtbl = &BitmapScaler_Vtbl;
    This->ref = 1;
    This->source = NULL;
    This->width = 0;
    This->height = 0;
    This->mode = 0;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BitmapScaler.lock");

    *scaler = &This->IWICBitmapScaler_iface;

    return S_OK;
}
