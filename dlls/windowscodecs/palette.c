/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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
#include "winreg.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICPaletteVtbl *lpIWICPaletteVtbl;
    LONG ref;
} PaletteImpl;

static HRESULT WINAPI PaletteImpl_QueryInterface(IWICPalette *iface, REFIID iid,
    void **ppv)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICPalette, iid))
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

static ULONG WINAPI PaletteImpl_AddRef(IWICPalette *iface)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PaletteImpl_Release(IWICPalette *iface)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI PaletteImpl_InitializePredefined(IWICPalette *iface,
    WICBitmapPaletteType ePaletteType, BOOL fAddTransparentColor)
{
    FIXME("(%p,%u,%i): stub\n", iface, ePaletteType, fAddTransparentColor);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_InitializeCustom(IWICPalette *iface,
    WICColor *pColors, UINT colorCount)
{
    FIXME("(%p,%p,%u): stub\n", iface, pColors, colorCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_InitializeFromBitmap(IWICPalette *iface,
    IWICBitmapSource *pISurface, UINT colorCount, BOOL fAddTransparentColor)
{
    FIXME("(%p,%p,%u,%i): stub\n", iface, pISurface, colorCount, fAddTransparentColor);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_InitializeFromPalette(IWICPalette *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_GetType(IWICPalette *iface,
    WICBitmapPaletteType *pePaletteType)
{
    FIXME("(%p,%p): stub\n", iface, pePaletteType);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_GetColorCount(IWICPalette *iface, UINT *pcCount)
{
    FIXME("(%p,%p): stub\n", iface, pcCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_GetColors(IWICPalette *iface, UINT colorCount,
    WICColor *pColors, UINT *pcActualColors)
{
    FIXME("(%p,%i,%p,%p): stub\n", iface, colorCount, pColors, pcActualColors);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_IsBlackWhite(IWICPalette *iface, BOOL *pfIsBlackWhite)
{
    FIXME("(%p,%p): stub\n", iface, pfIsBlackWhite);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_IsGrayscale(IWICPalette *iface, BOOL *pfIsGrayscale)
{
    FIXME("(%p,%p): stub\n", iface, pfIsGrayscale);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_HasAlpha(IWICPalette *iface, BOOL *pfHasAlpha)
{
    FIXME("(%p,%p): stub\n", iface, pfHasAlpha);
    return E_NOTIMPL;
}

static const IWICPaletteVtbl PaletteImpl_Vtbl = {
    PaletteImpl_QueryInterface,
    PaletteImpl_AddRef,
    PaletteImpl_Release,
    PaletteImpl_InitializePredefined,
    PaletteImpl_InitializeCustom,
    PaletteImpl_InitializeFromBitmap,
    PaletteImpl_InitializeFromPalette,
    PaletteImpl_GetType,
    PaletteImpl_GetColorCount,
    PaletteImpl_GetColors,
    PaletteImpl_IsBlackWhite,
    PaletteImpl_IsGrayscale,
    PaletteImpl_HasAlpha
};

HRESULT PaletteImpl_Create(IWICPalette **palette)
{
    PaletteImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PaletteImpl));
    if (!This) return E_OUTOFMEMORY;

    This->lpIWICPaletteVtbl = &PaletteImpl_Vtbl;
    This->ref = 1;

    *palette = (IWICPalette*)This;

    return S_OK;
}
