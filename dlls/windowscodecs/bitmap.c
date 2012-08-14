/*
 * Copyright 2012 Vincent Povirk for CodeWeavers
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

typedef struct BitmapImpl {
    IWICBitmap IWICBitmap_iface;
    LONG ref;
    IWICPalette *palette;
    int palette_set;
} BitmapImpl;

static inline BitmapImpl *impl_from_IWICBitmap(IWICBitmap *iface)
{
    return CONTAINING_RECORD(iface, BitmapImpl, IWICBitmap_iface);
}

static HRESULT WINAPI BitmapImpl_QueryInterface(IWICBitmap *iface, REFIID iid,
    void **ppv)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmap, iid))
    {
        *ppv = &This->IWICBitmap_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapImpl_AddRef(IWICBitmap *iface)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapImpl_Release(IWICBitmap *iface)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->palette) IWICPalette_Release(This->palette);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapImpl_GetSize(IWICBitmap *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FIXME("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapImpl_GetPixelFormat(IWICBitmap *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FIXME("(%p,%p)\n", iface, pPixelFormat);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapImpl_GetResolution(IWICBitmap *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapImpl_CopyPalette(IWICBitmap *iface,
    IWICPalette *pIPalette)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->palette_set)
        return WINCODEC_ERR_PALETTEUNAVAILABLE;

    return IWICPalette_InitializeFromPalette(pIPalette, This->palette);
}

static HRESULT WINAPI BitmapImpl_CopyPixels(IWICBitmap *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapImpl_Lock(IWICBitmap *iface, const WICRect *prcLock,
    DWORD flags, IWICBitmapLock **ppILock)
{
    FIXME("(%p,%p,%x,%p)\n", iface, prcLock, flags, ppILock);

    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapImpl_SetPalette(IWICBitmap *iface, IWICPalette *pIPalette)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->palette)
    {
        IWICPalette *new_palette;
        hr = PaletteImpl_Create(&new_palette);

        if (FAILED(hr)) return hr;

        if (InterlockedCompareExchangePointer((void**)&This->palette, new_palette, NULL))
        {
            /* someone beat us to it */
            IWICPalette_Release(new_palette);
        }
    }

    hr = IWICPalette_InitializeFromPalette(This->palette, pIPalette);

    if (SUCCEEDED(hr))
        This->palette_set = 1;

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_SetResolution(IWICBitmap *iface,
    double dpiX, double dpiY)
{
    FIXME("(%p,%f,%f)\n", iface, dpiX, dpiY);

    return E_NOTIMPL;
}

static const IWICBitmapVtbl BitmapImpl_Vtbl = {
    BitmapImpl_QueryInterface,
    BitmapImpl_AddRef,
    BitmapImpl_Release,
    BitmapImpl_GetSize,
    BitmapImpl_GetPixelFormat,
    BitmapImpl_GetResolution,
    BitmapImpl_CopyPalette,
    BitmapImpl_CopyPixels,
    BitmapImpl_Lock,
    BitmapImpl_SetPalette,
    BitmapImpl_SetResolution
};

HRESULT BitmapImpl_Create(UINT uiWidth, UINT uiHeight,
    REFWICPixelFormatGUID pixelFormat, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap)
{
    BitmapImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapImpl));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmap_iface.lpVtbl = &BitmapImpl_Vtbl;
    This->ref = 1;
    This->palette = NULL;
    This->palette_set = 0;

    *ppIBitmap = &This->IWICBitmap_iface;

    return S_OK;
}
