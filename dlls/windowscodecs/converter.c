/*
 * Copyright 2009 Vincent Povirk
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

typedef struct FormatConverter {
    const IWICFormatConverterVtbl *lpVtbl;
    LONG ref;
} FormatConverter;

static HRESULT WINAPI FormatConverter_QueryInterface(IWICFormatConverter *iface, REFIID iid,
    void **ppv)
{
    FormatConverter *This = (FormatConverter*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICFormatConverter, iid))
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

static ULONG WINAPI FormatConverter_AddRef(IWICFormatConverter *iface)
{
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverter_Release(IWICFormatConverter *iface)
{
    FormatConverter *This = (FormatConverter*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI FormatConverter_GetSize(IWICFormatConverter *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FIXME("(%p,%p,%p): stub\n", iface, puiWidth, puiHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_GetPixelFormat(IWICFormatConverter *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FIXME("(%p,%p): stub\n", iface, pPixelFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_GetResolution(IWICFormatConverter *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_CopyPalette(IWICFormatConverter *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_CopyPixels(IWICFormatConverter *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p): stub\n", iface, prc, cbStride, cbBufferSize, pbBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_Initialize(IWICFormatConverter *iface,
    IWICBitmapSource *pISource, REFWICPixelFormatGUID dstFormat, WICBitmapDitherType dither,
    IWICPalette *pIPalette, double alphaThresholdPercent, WICBitmapPaletteType paletteTranslate)
{
    FIXME("(%p,%p,%s,%u,%p,%0.1f,%u): stub\n", iface, pISource, debugstr_guid(dstFormat),
        dither, pIPalette, alphaThresholdPercent, paletteTranslate);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverter_CanConvert(IWICFormatConverter *iface,
    REFWICPixelFormatGUID srcPixelFormat, REFWICPixelFormatGUID dstPixelFormat,
    BOOL *pfCanConvert)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(srcPixelFormat),
        debugstr_guid(dstPixelFormat), pfCanConvert);
    return E_NOTIMPL;
}

static const IWICFormatConverterVtbl FormatConverter_Vtbl = {
    FormatConverter_QueryInterface,
    FormatConverter_AddRef,
    FormatConverter_Release,
    FormatConverter_GetSize,
    FormatConverter_GetPixelFormat,
    FormatConverter_GetResolution,
    FormatConverter_CopyPalette,
    FormatConverter_CopyPixels,
    FormatConverter_Initialize,
    FormatConverter_CanConvert
};

HRESULT FormatConverter_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    FormatConverter *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FormatConverter));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &FormatConverter_Vtbl;
    This->ref = 1;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
