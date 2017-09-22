/*
 *    GDI Interop
 *
 * Copyright 2011 Huw Davies
 * Copyright 2012, 2014-2016 Nikolay Sivov for CodeWeavers
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

struct dib_data {
    DWORD *ptr;
    int stride;
    int width;
};

struct rendertarget {
    IDWriteBitmapRenderTarget1 IDWriteBitmapRenderTarget1_iface;
    ID2D1SimplifiedGeometrySink ID2D1SimplifiedGeometrySink_iface;
    LONG ref;

    IDWriteFactory5 *factory;
    DWRITE_TEXT_ANTIALIAS_MODE antialiasmode;
    FLOAT ppdip;
    DWRITE_MATRIX m;
    SIZE size;
    HDC hdc;
    struct dib_data dib;
};

struct gdiinterop {
    IDWriteGdiInterop1 IDWriteGdiInterop1_iface;
    LONG ref;
    IDWriteFactory5 *factory;
};

static inline int get_dib_stride(int width, int bpp)
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static HRESULT create_target_dibsection(struct rendertarget *target, UINT32 width, UINT32 height)
{
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmi = (BITMAPINFO*)bmibuf;
    HBITMAP hbm;

    target->size.cx = width;
    target->size.cy = height;

    memset(bmi, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = -height;
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(target->hdc, bmi, DIB_RGB_COLORS, (void**)&target->dib.ptr, NULL, 0);
    if (!hbm) {
        hbm = CreateBitmap(1, 1, 1, 1, NULL);
        target->dib.ptr = NULL;
        target->dib.stride = 0;
        target->dib.width = 0;
    }
    else {
        target->dib.stride = get_dib_stride(width, 32);
        target->dib.width = width;
    }

    DeleteObject(SelectObject(target->hdc, hbm));
    return S_OK;
}

static inline struct rendertarget *impl_from_IDWriteBitmapRenderTarget1(IDWriteBitmapRenderTarget1 *iface)
{
    return CONTAINING_RECORD(iface, struct rendertarget, IDWriteBitmapRenderTarget1_iface);
}

static inline struct rendertarget *impl_from_ID2D1SimplifiedGeometrySink(ID2D1SimplifiedGeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct rendertarget, ID2D1SimplifiedGeometrySink_iface);
}

static inline struct gdiinterop *impl_from_IDWriteGdiInterop1(IDWriteGdiInterop1 *iface)
{
    return CONTAINING_RECORD(iface, struct gdiinterop, IDWriteGdiInterop1_iface);
}

static HRESULT WINAPI rendertarget_sink_QueryInterface(ID2D1SimplifiedGeometrySink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_ID2D1SimplifiedGeometrySink) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ID2D1SimplifiedGeometrySink_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI rendertarget_sink_AddRef(ID2D1SimplifiedGeometrySink *iface)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    return IDWriteBitmapRenderTarget1_AddRef(&This->IDWriteBitmapRenderTarget1_iface);
}

static ULONG WINAPI rendertarget_sink_Release(ID2D1SimplifiedGeometrySink *iface)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    return IDWriteBitmapRenderTarget1_Release(&This->IDWriteBitmapRenderTarget1_iface);
}

static void WINAPI rendertarget_sink_SetFillMode(ID2D1SimplifiedGeometrySink *iface, D2D1_FILL_MODE mode)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    SetPolyFillMode(This->hdc, mode == D2D1_FILL_MODE_ALTERNATE ? ALTERNATE : WINDING);
}

static void WINAPI rendertarget_sink_SetSegmentFlags(ID2D1SimplifiedGeometrySink *iface, D2D1_PATH_SEGMENT vertexFlags)
{
}

static void WINAPI rendertarget_sink_BeginFigure(ID2D1SimplifiedGeometrySink *iface, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    MoveToEx(This->hdc, startPoint.x, startPoint.y, NULL);
}

static void WINAPI rendertarget_sink_AddLines(ID2D1SimplifiedGeometrySink *iface, const D2D1_POINT_2F *points, UINT32 count)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);

    while (count--) {
        LineTo(This->hdc, points->x, points->y);
        points++;
    }
}

static void WINAPI rendertarget_sink_AddBeziers(ID2D1SimplifiedGeometrySink *iface, const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    POINT points[3];

    while (count--) {
        points[0].x = beziers->point1.x;
        points[0].y = beziers->point1.y;
        points[1].x = beziers->point2.x;
        points[1].y = beziers->point2.y;
        points[2].x = beziers->point3.x;
        points[2].y = beziers->point3.y;

        PolyBezierTo(This->hdc, points, 3);
        beziers++;
    }
}

static void WINAPI rendertarget_sink_EndFigure(ID2D1SimplifiedGeometrySink *iface, D2D1_FIGURE_END figureEnd)
{
    struct rendertarget *This = impl_from_ID2D1SimplifiedGeometrySink(iface);
    CloseFigure(This->hdc);
}

static HRESULT WINAPI rendertarget_sink_Close(ID2D1SimplifiedGeometrySink *iface)
{
    return S_OK;
}

static const ID2D1SimplifiedGeometrySinkVtbl rendertargetsinkvtbl = {
    rendertarget_sink_QueryInterface,
    rendertarget_sink_AddRef,
    rendertarget_sink_Release,
    rendertarget_sink_SetFillMode,
    rendertarget_sink_SetSegmentFlags,
    rendertarget_sink_BeginFigure,
    rendertarget_sink_AddLines,
    rendertarget_sink_AddBeziers,
    rendertarget_sink_EndFigure,
    rendertarget_sink_Close
};

static HRESULT WINAPI rendertarget_QueryInterface(IDWriteBitmapRenderTarget1 *iface, REFIID riid, void **obj)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteBitmapRenderTarget1) ||
        IsEqualIID(riid, &IID_IDWriteBitmapRenderTarget) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteBitmapRenderTarget1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI rendertarget_AddRef(IDWriteBitmapRenderTarget1 *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI rendertarget_Release(IDWriteBitmapRenderTarget1 *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDWriteFactory5_Release(This->factory);
        DeleteDC(This->hdc);
        heap_free(This);
    }

    return ref;
}

static inline DWORD *get_pixel_ptr_32(struct dib_data *dib, int x, int y)
{
    return (DWORD *)((BYTE*)dib->ptr + y * dib->stride + x * 4);
}

static inline BYTE blend_color(BYTE dst, BYTE src, BYTE alpha)
{
    return (src * alpha + dst * (255 - alpha) + 127) / 255;
}

static inline DWORD blend_subpixel(BYTE r, BYTE g, BYTE b, DWORD text, const BYTE *alpha)
{
    return blend_color(r, text >> 16, alpha[0]) << 16 |
           blend_color(g, text >> 8,  alpha[1]) << 8  |
           blend_color(b, text,       alpha[2]);
}

static inline DWORD blend_pixel(BYTE r, BYTE g, BYTE b, DWORD text, BYTE alpha)
{
    return blend_color(r, text >> 16, alpha) << 16 |
           blend_color(g, text >> 8,  alpha) << 8  |
           blend_color(b, text,       alpha);
}

static void blit_8(struct dib_data *dib, const BYTE *src, const RECT *rect, DWORD text_pixel)
{
    DWORD *dst_ptr = get_pixel_ptr_32(dib, rect->left, rect->top);
    int x, y, src_width = rect->right - rect->left;

    for (y = rect->top; y < rect->bottom; y++) {
        for (x = 0; x < src_width; x++) {
            if (!src[x]) continue;
            if (src[x] == DWRITE_ALPHA_MAX)
                dst_ptr[x] = text_pixel;
            else
                dst_ptr[x] = blend_pixel(dst_ptr[x] >> 16, dst_ptr[x] >> 8, dst_ptr[x], text_pixel, src[x]);
        }

        src += src_width;
        dst_ptr += dib->stride / 4;
    }
}

static void blit_subpixel_888(struct dib_data *dib, int dib_width, const BYTE *src,
    const RECT *rect, DWORD text_pixel)
{
    DWORD *dst_ptr = get_pixel_ptr_32(dib, rect->left, rect->top);
    int x, y, src_width = rect->right - rect->left;

    for (y = rect->top; y < rect->bottom; y++) {
        for (x = 0; x < src_width; x++) {
            if (src[3*x] == 0 && src[3*x+1] == 0 && src[3*x+2] == 0) continue;
            dst_ptr[x] = blend_subpixel(dst_ptr[x] >> 16, dst_ptr[x] >> 8, dst_ptr[x], text_pixel, &src[3*x]);
        }
        dst_ptr += dib->stride / 4;
        src += src_width * 3;
    }
}

static inline DWORD colorref_to_pixel_888(COLORREF color)
{
    return (((color >> 16) & 0xff) | (color & 0xff00) | ((color << 16) & 0xff0000));
}

static HRESULT WINAPI rendertarget_DrawGlyphRun(IDWriteBitmapRenderTarget1 *iface,
    FLOAT originX, FLOAT originY, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GLYPH_RUN const *run, IDWriteRenderingParams *params, COLORREF color,
    RECT *bbox_ret)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    IDWriteGlyphRunAnalysis *analysis;
    DWRITE_RENDERING_MODE1 rendermode;
    DWRITE_GRID_FIT_MODE gridfitmode;
    DWRITE_TEXTURE_TYPE texturetype;
    DWRITE_GLYPH_RUN scaled_run;
    IDWriteFontFace3 *fontface;
    RECT target, bounds;
    HRESULT hr;

    TRACE("(%p)->(%.2f %.2f %d %p %p 0x%08x %p)\n", This, originX, originY,
        measuring_mode, run, params, color, bbox_ret);

    SetRectEmpty(bbox_ret);

    if (!This->dib.ptr)
        return S_OK;

    if (!params)
        return E_INVALIDARG;

    if (FAILED(hr = IDWriteFontFace_QueryInterface(run->fontFace, &IID_IDWriteFontFace3, (void **)&fontface))) {
        WARN("Failed to get IDWriteFontFace2 interface, hr %#x.\n", hr);
        return hr;
    }

    hr = IDWriteFontFace3_GetRecommendedRenderingMode(fontface, run->fontEmSize, This->ppdip * 96.0f,
            This->ppdip * 96.0f, NULL /* FIXME */, run->isSideways, DWRITE_OUTLINE_THRESHOLD_ALIASED, measuring_mode,
            params, &rendermode, &gridfitmode);
    IDWriteFontFace3_Release(fontface);
    if (FAILED(hr))
        return hr;

    SetRect(&target, 0, 0, This->size.cx, This->size.cy);

    if (rendermode == DWRITE_RENDERING_MODE1_OUTLINE) {
        static const XFORM identity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
        const DWRITE_MATRIX *m = &This->m;
        XFORM xform;

        /* target allows any transform to be set, filter it here */
        if (m->m11 * m->m22 == m->m12 * m->m21) {
            xform.eM11 = 1.0f;
            xform.eM12 = 0.0f;
            xform.eM21 = 0.0f;
            xform.eM22 = 1.0f;
            xform.eDx  = originX;
            xform.eDy  = originY;
        } else {
            xform.eM11 = m->m11;
            xform.eM12 = m->m12;
            xform.eM21 = m->m21;
            xform.eM22 = m->m22;
            xform.eDx  = m->m11 * originX + m->m21 * originY + m->dx;
            xform.eDy  = m->m12 * originX + m->m22 * originY + m->dy;
        }
        SetWorldTransform(This->hdc, &xform);

        BeginPath(This->hdc);

        hr = IDWriteFontFace_GetGlyphRunOutline(run->fontFace, run->fontEmSize * This->ppdip,
            run->glyphIndices, run->glyphAdvances, run->glyphOffsets, run->glyphCount,
            run->isSideways, run->bidiLevel & 1, &This->ID2D1SimplifiedGeometrySink_iface);

        EndPath(This->hdc);

        if (hr == S_OK) {
            HBRUSH brush = CreateSolidBrush(color);

            SelectObject(This->hdc, brush);

            FillPath(This->hdc);

            /* FIXME: one way to get affected rectangle bounds is to use region fill */
            if (bbox_ret)
                *bbox_ret = target;

            DeleteObject(brush);
        }

        SetWorldTransform(This->hdc, &identity);

        return hr;
    }

    scaled_run = *run;
    scaled_run.fontEmSize *= This->ppdip;
    hr = IDWriteFactory5_CreateGlyphRunAnalysis(This->factory, &scaled_run, &This->m, rendermode, measuring_mode,
            gridfitmode, This->antialiasmode, originX, originY, &analysis);
    if (FAILED(hr)) {
        WARN("failed to create analysis instance, 0x%08x\n", hr);
        return hr;
    }

    SetRectEmpty(&bounds);
    texturetype = DWRITE_TEXTURE_ALIASED_1x1;
    hr = IDWriteGlyphRunAnalysis_GetAlphaTextureBounds(analysis, DWRITE_TEXTURE_ALIASED_1x1, &bounds);
    if (FAILED(hr) || IsRectEmpty(&bounds)) {
        hr = IDWriteGlyphRunAnalysis_GetAlphaTextureBounds(analysis, DWRITE_TEXTURE_CLEARTYPE_3x1, &bounds);
        if (FAILED(hr)) {
            WARN("GetAlphaTextureBounds() failed, 0x%08x\n", hr);
            IDWriteGlyphRunAnalysis_Release(analysis);
            return hr;
        }
        texturetype = DWRITE_TEXTURE_CLEARTYPE_3x1;
    }

    if (IntersectRect(&target, &target, &bounds)) {
        UINT32 size = (target.right - target.left) * (target.bottom - target.top);
        BYTE *bitmap;

        color = colorref_to_pixel_888(color);
        if (texturetype == DWRITE_TEXTURE_CLEARTYPE_3x1)
            size *= 3;
        bitmap = heap_alloc_zero(size);
        if (!bitmap) {
            IDWriteGlyphRunAnalysis_Release(analysis);
            return E_OUTOFMEMORY;
        }

        hr = IDWriteGlyphRunAnalysis_CreateAlphaTexture(analysis, texturetype, &target, bitmap, size);
        if (hr == S_OK) {
            /* blit to target dib */
            if (texturetype == DWRITE_TEXTURE_ALIASED_1x1)
                blit_8(&This->dib, bitmap, &target, color);
            else
                blit_subpixel_888(&This->dib, This->size.cx, bitmap, &target, color);

            if (bbox_ret) *bbox_ret = target;
        }

        heap_free(bitmap);
    }

    IDWriteGlyphRunAnalysis_Release(analysis);

    return S_OK;
}

static HDC WINAPI rendertarget_GetMemoryDC(IDWriteBitmapRenderTarget1 *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    TRACE("(%p)\n", This);
    return This->hdc;
}

static FLOAT WINAPI rendertarget_GetPixelsPerDip(IDWriteBitmapRenderTarget1 *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    TRACE("(%p)\n", This);
    return This->ppdip;
}

static HRESULT WINAPI rendertarget_SetPixelsPerDip(IDWriteBitmapRenderTarget1 *iface, FLOAT ppdip)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%.2f)\n", This, ppdip);

    if (ppdip <= 0.0f)
        return E_INVALIDARG;

    This->ppdip = ppdip;
    return S_OK;
}

static HRESULT WINAPI rendertarget_GetCurrentTransform(IDWriteBitmapRenderTarget1 *iface, DWRITE_MATRIX *transform)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    *transform = This->m;
    return S_OK;
}

static HRESULT WINAPI rendertarget_SetCurrentTransform(IDWriteBitmapRenderTarget1 *iface, DWRITE_MATRIX const *transform)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    This->m = transform ? *transform : identity;
    return S_OK;
}

static HRESULT WINAPI rendertarget_GetSize(IDWriteBitmapRenderTarget1 *iface, SIZE *size)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%p)\n", This, size);
    *size = This->size;
    return S_OK;
}

static HRESULT WINAPI rendertarget_Resize(IDWriteBitmapRenderTarget1 *iface, UINT32 width, UINT32 height)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%u %u)\n", This, width, height);

    if (This->size.cx == width && This->size.cy == height)
        return S_OK;

    return create_target_dibsection(This, width, height);
}

static DWRITE_TEXT_ANTIALIAS_MODE WINAPI rendertarget_GetTextAntialiasMode(IDWriteBitmapRenderTarget1 *iface)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);
    TRACE("(%p)\n", This);
    return This->antialiasmode;
}

static HRESULT WINAPI rendertarget_SetTextAntialiasMode(IDWriteBitmapRenderTarget1 *iface, DWRITE_TEXT_ANTIALIAS_MODE mode)
{
    struct rendertarget *This = impl_from_IDWriteBitmapRenderTarget1(iface);

    TRACE("(%p)->(%d)\n", This, mode);

    if ((DWORD)mode > DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE)
        return E_INVALIDARG;

    This->antialiasmode = mode;
    return S_OK;
}

static const IDWriteBitmapRenderTarget1Vtbl rendertargetvtbl = {
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
    rendertarget_Resize,
    rendertarget_GetTextAntialiasMode,
    rendertarget_SetTextAntialiasMode
};

static HRESULT create_rendertarget(IDWriteFactory5 *factory, HDC hdc, UINT32 width, UINT32 height, IDWriteBitmapRenderTarget **ret)
{
    struct rendertarget *target;
    HRESULT hr;

    *ret = NULL;

    target = heap_alloc(sizeof(struct rendertarget));
    if (!target) return E_OUTOFMEMORY;

    target->IDWriteBitmapRenderTarget1_iface.lpVtbl = &rendertargetvtbl;
    target->ID2D1SimplifiedGeometrySink_iface.lpVtbl = &rendertargetsinkvtbl;
    target->ref = 1;

    target->hdc = CreateCompatibleDC(hdc);
    SetGraphicsMode(target->hdc, GM_ADVANCED);
    hr = create_target_dibsection(target, width, height);
    if (FAILED(hr)) {
        IDWriteBitmapRenderTarget1_Release(&target->IDWriteBitmapRenderTarget1_iface);
        return hr;
    }

    target->m = identity;
    target->ppdip = GetDeviceCaps(target->hdc, LOGPIXELSX) / 96.0f;
    target->antialiasmode = DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    target->factory = factory;
    IDWriteFactory5_AddRef(factory);

    *ret = (IDWriteBitmapRenderTarget*)&target->IDWriteBitmapRenderTarget1_iface;

    return S_OK;
}

static HRESULT WINAPI gdiinterop_QueryInterface(IDWriteGdiInterop1 *iface, REFIID riid, void **obj)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteGdiInterop1) ||
        IsEqualIID(riid, &IID_IDWriteGdiInterop) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteGdiInterop1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gdiinterop_AddRef(IDWriteGdiInterop1 *iface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI gdiinterop_Release(IDWriteGdiInterop1 *iface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        factory_detach_gdiinterop(This->factory, iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI gdiinterop_CreateFontFromLOGFONT(IDWriteGdiInterop1 *iface,
    LOGFONTW const *logfont, IDWriteFont **font)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    TRACE("(%p)->(%p %p)\n", This, logfont, font);

    return IDWriteGdiInterop1_CreateFontFromLOGFONT(iface, logfont, NULL, font);
}

static HRESULT WINAPI gdiinterop_ConvertFontToLOGFONT(IDWriteGdiInterop1 *iface,
    IDWriteFont *font, LOGFONTW *logfont, BOOL *is_systemfont)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    HRESULT hr;

    TRACE("(%p)->(%p %p %p)\n", This, font, logfont, is_systemfont);

    *is_systemfont = FALSE;

    memset(logfont, 0, sizeof(*logfont));

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

    get_logfont_from_font(font, logfont);
    logfont->lfCharSet = DEFAULT_CHARSET;
    logfont->lfOutPrecision = OUT_OUTLINE_PRECIS;

    return hr;
}

static HRESULT WINAPI gdiinterop_ConvertFontFaceToLOGFONT(IDWriteGdiInterop1 *iface,
    IDWriteFontFace *fontface, LOGFONTW *logfont)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    TRACE("(%p)->(%p %p)\n", This, fontface, logfont);

    memset(logfont, 0, sizeof(*logfont));

    if (!fontface)
        return E_INVALIDARG;

    get_logfont_from_fontface(fontface, logfont);
    logfont->lfCharSet = DEFAULT_CHARSET;
    logfont->lfOutPrecision = OUT_OUTLINE_PRECIS;

    return S_OK;
}

struct font_realization_info {
    DWORD size;
    DWORD flags;
    DWORD cache_num;
    DWORD instance_id;
    DWORD unk;
    WORD  face_index;
    WORD  simulations;
};

struct font_fileinfo {
    FILETIME writetime;
    LARGE_INTEGER size;
    WCHAR path[1];
};

/* Undocumented gdi32 exports, used to access actually selected font information */
extern BOOL WINAPI GetFontRealizationInfo(HDC hdc, struct font_realization_info *info);
extern BOOL WINAPI GetFontFileInfo(DWORD instance_id, DWORD unknown, struct font_fileinfo *info, DWORD size, DWORD *needed);

static HRESULT WINAPI gdiinterop_CreateFontFaceFromHdc(IDWriteGdiInterop1 *iface,
    HDC hdc, IDWriteFontFace **fontface)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    struct font_realization_info info;
    struct font_fileinfo *fileinfo;
    DWRITE_FONT_FILE_TYPE filetype;
    DWRITE_FONT_FACE_TYPE facetype;
    IDWriteFontFile *file;
    BOOL is_supported;
    UINT32 facenum;
    DWORD needed;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, hdc, fontface);

    *fontface = NULL;

    if (!hdc)
        return E_INVALIDARG;

    /* get selected font id  */
    info.size = sizeof(info);
    if (!GetFontRealizationInfo(hdc, &info)) {
        WARN("failed to get selected font id\n");
        return E_FAIL;
    }

    needed = 0;
    GetFontFileInfo(info.instance_id, 0, NULL, 0, &needed);
    if (needed == 0) {
        WARN("failed to get font file info size\n");
        return E_FAIL;
    }

    fileinfo = heap_alloc(needed);
    if (!fileinfo)
        return E_OUTOFMEMORY;

    if (!GetFontFileInfo(info.instance_id, 0, fileinfo, needed, &needed)) {
        heap_free(fileinfo);
        return E_FAIL;
    }

    hr = IDWriteFactory5_CreateFontFileReference(This->factory, fileinfo->path, &fileinfo->writetime,
        &file);
    heap_free(fileinfo);
    if (FAILED(hr))
        return hr;

    is_supported = FALSE;
    hr = IDWriteFontFile_Analyze(file, &is_supported, &filetype, &facetype, &facenum);
    if (SUCCEEDED(hr)) {
        if (is_supported)
            /* Simulations flags values match DWRITE_FONT_SIMULATIONS */
            hr = IDWriteFactory5_CreateFontFace(This->factory, facetype, 1, &file, info.face_index,
                    info.simulations, fontface);
        else
            hr = DWRITE_E_FILEFORMAT;
    }

    IDWriteFontFile_Release(file);
    return hr;
}

static HRESULT WINAPI gdiinterop_CreateBitmapRenderTarget(IDWriteGdiInterop1 *iface,
    HDC hdc, UINT32 width, UINT32 height, IDWriteBitmapRenderTarget **target)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    TRACE("(%p)->(%p %u %u %p)\n", This, hdc, width, height, target);
    return create_rendertarget(This->factory, hdc, width, height, target);
}

static HRESULT WINAPI gdiinterop1_CreateFontFromLOGFONT(IDWriteGdiInterop1 *iface,
    LOGFONTW const *logfont, IDWriteFontCollection *collection, IDWriteFont **font)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);
    IDWriteFontFamily *family;
    DWRITE_FONT_STYLE style;
    BOOL exists = FALSE;
    UINT32 index;
    HRESULT hr;

    TRACE("(%p)->(%p %p %p)\n", This, logfont, collection, font);

    *font = NULL;

    if (!logfont) return E_INVALIDARG;

    if (collection)
        IDWriteFontCollection_AddRef(collection);
    else {
        hr = IDWriteFactory5_GetSystemFontCollection(This->factory, FALSE, (IDWriteFontCollection1**)&collection, FALSE);
        if (FAILED(hr)) {
            ERR("failed to get system font collection: 0x%08x.\n", hr);
            return hr;
        }
    }

    hr = IDWriteFontCollection_FindFamilyName(collection, logfont->lfFaceName, &index, &exists);
    if (FAILED(hr))
        goto done;

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

static HRESULT WINAPI gdiinterop1_GetFontSignature_(IDWriteGdiInterop1 *iface, IDWriteFontFace *fontface,
    FONTSIGNATURE *fontsig)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    TRACE("(%p)->(%p %p)\n", This, fontface, fontsig);

    return get_fontsig_from_fontface(fontface, fontsig);
}

static HRESULT WINAPI gdiinterop1_GetFontSignature(IDWriteGdiInterop1 *iface, IDWriteFont *font, FONTSIGNATURE *fontsig)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    TRACE("(%p)->(%p %p)\n", This, font, fontsig);

    if (!font)
        return E_INVALIDARG;

    return get_fontsig_from_font(font, fontsig);
}

static HRESULT WINAPI gdiinterop1_GetMatchingFontsByLOGFONT(IDWriteGdiInterop1 *iface, LOGFONTW const *logfont,
    IDWriteFontSet *fontset, IDWriteFontSet **subset)
{
    struct gdiinterop *This = impl_from_IDWriteGdiInterop1(iface);

    FIXME("(%p)->(%p %p %p): stub\n", This, logfont, fontset, subset);

    return E_NOTIMPL;
}

static const struct IDWriteGdiInterop1Vtbl gdiinteropvtbl = {
    gdiinterop_QueryInterface,
    gdiinterop_AddRef,
    gdiinterop_Release,
    gdiinterop_CreateFontFromLOGFONT,
    gdiinterop_ConvertFontToLOGFONT,
    gdiinterop_ConvertFontFaceToLOGFONT,
    gdiinterop_CreateFontFaceFromHdc,
    gdiinterop_CreateBitmapRenderTarget,
    gdiinterop1_CreateFontFromLOGFONT,
    gdiinterop1_GetFontSignature_,
    gdiinterop1_GetFontSignature,
    gdiinterop1_GetMatchingFontsByLOGFONT
};

HRESULT create_gdiinterop(IDWriteFactory5 *factory, IDWriteGdiInterop1 **ret)
{
    struct gdiinterop *interop;

    *ret = NULL;

    if (!(interop = heap_alloc(sizeof(*interop))))
        return E_OUTOFMEMORY;

    interop->IDWriteGdiInterop1_iface.lpVtbl = &gdiinteropvtbl;
    interop->ref = 1;
    IDWriteFactory5_AddRef(interop->factory = factory);

    *ret = &interop->IDWriteGdiInterop1_iface;
    return S_OK;
}
