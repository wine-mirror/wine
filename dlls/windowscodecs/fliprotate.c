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

typedef struct FlipRotator {
    const IWICBitmapFlipRotatorVtbl *lpVtbl;
    LONG ref;
    CRITICAL_SECTION lock; /* must be held when initialized */
} FlipRotator;

static HRESULT WINAPI FlipRotator_QueryInterface(IWICBitmapFlipRotator *iface, REFIID iid,
    void **ppv)
{
    FlipRotator *This = (FlipRotator*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFlipRotator, iid))
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

static ULONG WINAPI FlipRotator_AddRef(IWICBitmapFlipRotator *iface)
{
    FlipRotator *This = (FlipRotator*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FlipRotator_Release(IWICBitmapFlipRotator *iface)
{
    FlipRotator *This = (FlipRotator*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI FlipRotator_GetSize(IWICBitmapFlipRotator *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FIXME("(%p,%p,%p): stub\n", iface, puiWidth, puiHeight);

    return E_NOTIMPL;
}

static HRESULT WINAPI FlipRotator_GetPixelFormat(IWICBitmapFlipRotator *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FIXME("(%p,%p): stub\n", iface, pPixelFormat);

    return E_NOTIMPL;
}

static HRESULT WINAPI FlipRotator_GetResolution(IWICBitmapFlipRotator *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);

    return E_NOTIMPL;
}

static HRESULT WINAPI FlipRotator_CopyPalette(IWICBitmapFlipRotator *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI FlipRotator_CopyPixels(IWICBitmapFlipRotator *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p): stub\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI FlipRotator_Initialize(IWICBitmapFlipRotator *iface,
    IWICBitmapSource *pISource, WICBitmapTransformOptions options)
{
    FIXME("(%p,%p,%u): stub\n", iface, pISource, options);

    return E_NOTIMPL;
}

static const IWICBitmapFlipRotatorVtbl FlipRotator_Vtbl = {
    FlipRotator_QueryInterface,
    FlipRotator_AddRef,
    FlipRotator_Release,
    FlipRotator_GetSize,
    FlipRotator_GetPixelFormat,
    FlipRotator_GetResolution,
    FlipRotator_CopyPalette,
    FlipRotator_CopyPixels,
    FlipRotator_Initialize
};

HRESULT FlipRotator_Create(IWICBitmapFlipRotator **fliprotator)
{
    FlipRotator *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FlipRotator));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &FlipRotator_Vtbl;
    This->ref = 1;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FlipRotator.lock");

    *fliprotator = (IWICBitmapFlipRotator*)This;

    return S_OK;
}
