/*
 *    GDI Interop
 *
 * Copyright 2012 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dwrite_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

static const DWRITE_MATRIX identity =
{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
};

struct gdiinterop {
    IDWriteGdiInterop IDWriteGdiInterop_iface;
    IDWriteFactory *factory;
};

struct rendertarget {
    IDWriteBitmapRenderTarget IDWriteBitmapRenderTarget_iface;
    LONG ref;

    FLOAT pixels_per_dip;
    DWRITE_MATRIX m;
    SIZE size;
    HDC hdc;
};

static HRESULT create_target_dibsection(HDC hdc, UINT32 width, UINT32 height)
{
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmi = (BITMAPINFO*)bmibuf;
    HBITMAP hbm;

    memset(bmi, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = height;
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    if (!hbm)
        hbm = CreateBitmap(1, 1, 1, 1, NULL);

    DeleteObject(SelectObject(hdc, hbm));
    return S_OK;
}

static inline struct rendertarget *impl_from_IDWriteBitmapRenderTarget(IDWriteBitmapRenderTarget *iface)
{
    return CONTAINING_RECORD(iface, struct rendertarget, IDWriteBitmapRenderTarget_iface);
}

static inline struct gdiinterop *impl_from_IDWriteGdiInterop(IDWriteGdiInterop *iface)
{
    return CONTAINING_RECORD(iface, struct gdiinterop, IDWriteGdiInterop_iface);
}

static HRESULT WINAPI rendertarget_QueryInterface(IDWriteBitmapRenderTarget *iface, REFIID riid, void **obj)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteBitmapRenderTarget))
    {
        *obj = iface;
        IDWriteBitmapRenderTarget_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI rendertarget_AddRef(IDWriteBitmapRenderTarget *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI rendertarget_Release(IDWriteBitmapRenderTarget *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        DeleteDC(This->hdc);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI rendertarget_DrawGlyphRun(IDWriteBitmapRenderTarget *iface,
    FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GLYPH_RUN const* glyph_run, IDWriteRenderingParams* params, COLORREF textColor,
    RECT *blackbox_rect)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);
    FIXME("(%p)->(%f %f %d %p %p 0x%08x %p): stub\n", This, baselineOriginX, baselineOriginY,
        measuring_mode, glyph_run, params, textColor, blackbox_rect);
    return E_NOTIMPL;
}

static HDC WINAPI rendertarget_GetMemoryDC(IDWriteBitmapRenderTarget *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);
    TRACE("(%p)\n", This);
    return This->hdc;
}

static FLOAT WINAPI rendertarget_GetPixelsPerDip(IDWriteBitmapRenderTarget *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);
    TRACE("(%p)\n", This);
    return This->pixels_per_dip;
}

static HRESULT WINAPI rendertarget_SetPixelsPerDip(IDWriteBitmapRenderTarget *iface, FLOAT pixels_per_dip)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%.2f)\n", This, pixels_per_dip);

    if (pixels_per_dip <= 0.0)
        return E_INVALIDARG;

    This->pixels_per_dip = pixels_per_dip;
    return S_OK;
}

static HRESULT WINAPI rendertarget_GetCurrentTransform(IDWriteBitmapRenderTarget *iface, DWRITE_MATRIX *transform)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    *transform = This->m;
    return S_OK;
}

static HRESULT WINAPI rendertarget_SetCurrentTransform(IDWriteBitmapRenderTarget *iface, DWRITE_MATRIX const *transform)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    This->m = transform ? *transform : identity;
    return S_OK;
}

static HRESULT WINAPI rendertarget_GetSize(IDWriteBitmapRenderTarget *iface, SIZE *size)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%p)\n", This, size);
    *size = This->size;
    return S_OK;
}

static HRESULT WINAPI rendertarget_Resize(IDWriteBitmapRenderTarget *iface, UINT32 width, UINT32 height)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget(iface);

    TRACE("(%p)->(%u %u)\n", This, width, height);

    if (This->size.cx == width && This->size.cy == height)
        return S_OK;

    return create_target_dibsection(This->hdc, width, height);
}

static const IDWriteBitmapRenderTargetVtbl rendertargetvtbl = {
    rendertarget_QueryInterface,
    rendertarget_AddRef,
    rendertarget_Release,
    rendertarget_DrawGlyphRun,
    rendertarget_GetMemoryDC,
    rendertarget_GetPixelsPerDip,
    rendertarget_SetPixelsPerDip,
    rendertarget_GetCurrentTransform,
    rendertarget_SetCurrentTransform,
    rendertarget_GetSize,
    rendertarget_Resize
};

static HRESULT create_rendertarget(HDC hdc, UINT32 width, UINT32 height, IDWriteBitmapRenderTarget **ret)
{
    struct rendertarget *target;
    HRESULT hr;

    *ret = NULL;

    target = heap_alloc(sizeof(struct rendertarget));
    if (!target) return E_OUTOFMEMORY;

    target->IDWriteBitmapRenderTarget_iface.lpVtbl = &rendertargetvtbl;
    target->ref = 1;

    target->size.cx = width;
    target->size.cy = height;

    target->hdc = CreateCompatibleDC(hdc);
    hr = create_target_dibsection(target->hdc, width, height);
    if (FAILED(hr)) {
        IDWriteBitmapRenderTarget_Release(&target->IDWriteBitmapRenderTarget_iface);
        return hr;
    }

    target->m = identity;
    target->pixels_per_dip = 1.0;

    *ret = &target->IDWriteBitmapRenderTarget_iface;

    return S_OK;
}

static HRESULT WINAPI gdiinterop_QueryInterface(IDWriteGdiInterop *iface, REFIID riid, void **obj)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteGdiInterop) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteGdiInterop_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gdiinterop_AddRef(IDWriteGdiInterop *iface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    TRACE("(%p)\n", This);
    return IDWriteFactory_AddRef(This->factory);
}

static ULONG WINAPI gdiinterop_Release(IDWriteGdiInterop *iface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    TRACE("(%p)\n", This);
    return IDWriteFactory_Release(This->factory);
}

static HRESULT WINAPI gdiinterop_CreateFontFromLOGFONT(IDWriteGdiInterop *iface,
    LOGFONTW const *logfont, IDWriteFont **font)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    DWRITE_FONT_STYLE style;
    BOOL exists = FALSE;
    UINT32 index;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, logfont, font);

    *font = NULL;

    if (!logfont) return E_INVALIDARG;

    hr = IDWriteFactory_GetSystemFontCollection(This->factory, &collection, FALSE);
    if (FAILED(hr)) {
        ERR("failed to get system font collection: 0x%08x.\n", hr);
        return hr;
    }

    hr = IDWriteFontCollection_FindFamilyName(collection, logfont->lfFaceName, &index, &exists);
    if (FAILED(hr)) {
        IDWriteFontCollection_Release(collection);
        goto done;
    }

    if (!exists) {
        hr = DWRITE_E_NOFONT;
        goto done;
    }

    hr = IDWriteFontCollection_GetFontFamily(collection, index, &family);
    if (FAILED(hr))
        goto done;

    style = logfont->lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    hr = IDWriteFontFamily_GetFirstMatchingFont(family, logfont->lfWeight, DWRITE_FONT_STRETCH_NORMAL, style, font);
    IDWriteFontFamily_Release(family);

done:
    IDWriteFontCollection_Release(collection);
    return hr;
}

static HRESULT WINAPI gdiinterop_ConvertFontToLOGFONT(IDWriteGdiInterop *iface,
    IDWriteFont *font, LOGFONTW *logfont, BOOL *is_systemfont)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    static const WCHAR enusW[] = {'e','n','-','u','s',0};
    DWRITE_FONT_SIMULATIONS simulations;
    IDWriteFontCollection *collection;
    IDWriteLocalizedStrings *name;
    IDWriteFontFamily *family;
    DWRITE_FONT_STYLE style;
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    TRACE("(%p)->(%p %p %p)\n", This, font, logfont, is_systemfont);

    *is_systemfont = FALSE;

    if (!font)
        return E_INVALIDARG;

    hr = IDWriteFont_GetFontFamily(font, &family);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFamily_GetFontCollection(family, &collection);
    IDWriteFontFamily_Release(family);
    if (FAILED(hr))
        return hr;

    *is_systemfont = is_system_collection(collection);
    IDWriteFontCollection_Release(collection);

    simulations = IDWriteFont_GetSimulations(font);
    style = IDWriteFont_GetStyle(font);

    logfont->lfCharSet = DEFAULT_CHARSET;
    logfont->lfWeight = IDWriteFont_GetWeight(font);
    logfont->lfItalic = style == DWRITE_FONT_STYLE_ITALIC || (simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE);
    logfont->lfOutPrecision = OUT_OUTLINE_PRECIS;
    logfont->lfFaceName[0] = 0;

    exists = FALSE;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &name, &exists);
    if (FAILED(hr) || !exists)
        return hr;

    IDWriteLocalizedStrings_FindLocaleName(name, enusW, &index, &exists);
    IDWriteLocalizedStrings_GetString(name, index, logfont->lfFaceName, sizeof(logfont->lfFaceName)/sizeof(WCHAR));
    IDWriteLocalizedStrings_Release(name);

    return S_OK;
}

static HRESULT WINAPI gdiinterop_ConvertFontFaceToLOGFONT(IDWriteGdiInterop *iface,
    IDWriteFontFace *fontface, LOGFONTW *logfont)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    TRACE("(%p)->(%p %p)\n", This, fontface, logfont);
    return convert_fontface_to_logfont(fontface, logfont);
}

static HRESULT WINAPI gdiinterop_CreateFontFaceFromHdc(IDWriteGdiInterop *iface,
    HDC hdc, IDWriteFontFace **fontface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    IDWriteFont *font;
    LOGFONTW logfont;
    HFONT hfont;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, hdc, fontface);

    *fontface = NULL;

    hfont = GetCurrentObject(hdc, OBJ_FONT);
    if (!hfont)
        return E_INVALIDARG;
    GetObjectW(hfont, sizeof(logfont), &logfont);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(iface, &logfont, &font);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFont_CreateFontFace(font, fontface);
    IDWriteFont_Release(font);

    return hr;
}

static HRESULT WINAPI gdiinterop_CreateBitmapRenderTarget(IDWriteGdiInterop *iface,
    HDC hdc, UINT32 width, UINT32 height, IDWriteBitmapRenderTarget **target)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop(iface);
    TRACE("(%p)->(%p %u %u %p)\n", This, hdc, width, height, target);
    return create_rendertarget(hdc, width, height, target);
}

static const struct IDWriteGdiInteropVtbl gdiinteropvtbl = {
    gdiinterop_QueryInterface,
    gdiinterop_AddRef,
    gdiinterop_Release,
    gdiinterop_CreateFontFromLOGFONT,
    gdiinterop_ConvertFontToLOGFONT,
    gdiinterop_ConvertFontFaceToLOGFONT,
    gdiinterop_CreateFontFaceFromHdc,
    gdiinterop_CreateBitmapRenderTarget
};

HRESULT create_gdiinterop(IDWriteFactory *factory, IDWriteGdiInterop **ret)
{
    struct gdiinterop *This;

    *ret = NULL;

    This = heap_alloc(sizeof(struct gdiinterop));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteGdiInterop_iface.lpVtbl = &gdiinteropvtbl;
    This->factory = factory;

    *ret= &This->IDWriteGdiInterop_iface;
    return S_OK;
}

void release_gdiinterop(IDWriteGdiInterop *iface)
{
    struct gdiinterop *interop = impl_from_IDWriteGdiInterop(iface);
    heap_free(interop);
}
