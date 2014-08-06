/*
 *    Font related tests
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

#include "windows.h"
#include "dwrite.h"

#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

static IDWriteFactory *factory;

static const WCHAR tahomaW[] = {'T','a','h','o','m','a',0};
static const WCHAR blahW[]  = {'B','l','a','h','!',0};

static void test_CreateFontFromLOGFONT(void)
{
    static const WCHAR tahomaspW[] = {'T','a','h','o','m','a',' ',0};
    IDWriteGdiInterop *interop;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    IDWriteFont *font;
    LOGFONTW logfont;
    LONG weights[][2] = {
        {FW_NORMAL, DWRITE_FONT_WEIGHT_NORMAL},
        {FW_BOLD, DWRITE_FONT_WEIGHT_BOLD},
        {  0, DWRITE_FONT_WEIGHT_NORMAL},
        { 50, DWRITE_FONT_WEIGHT_NORMAL},
        {150, DWRITE_FONT_WEIGHT_NORMAL},
        {250, DWRITE_FONT_WEIGHT_NORMAL},
        {350, DWRITE_FONT_WEIGHT_NORMAL},
        {450, DWRITE_FONT_WEIGHT_NORMAL},
        {650, DWRITE_FONT_WEIGHT_BOLD},
        {750, DWRITE_FONT_WEIGHT_BOLD},
        {850, DWRITE_FONT_WEIGHT_BOLD},
        {950, DWRITE_FONT_WEIGHT_BOLD},
        {960, DWRITE_FONT_WEIGHT_BOLD},
    };
    OUTLINETEXTMETRICW otm;
    HRESULT hr;
    BOOL ret;
    HDC hdc;
    HFONT hfont;
    int i;
    UINT r;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

if (0)
    /* null out parameter crashes this call */
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, NULL, NULL);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, NULL, &font);
    EXPECT_HR(hr, E_INVALIDARG);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    hfont = CreateFontIndirectW(&logfont);
    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    r = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    ok(r, "got %d\n", r);
    DeleteDC(hdc);
    DeleteObject(hfont);

    /* now check properties */
    weight = IDWriteFont_GetWeight(font);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);

    style = IDWriteFont_GetStyle(font);
todo_wine {
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);
    ok(otm.otmfsSelection == 1, "got 0x%08x\n", otm.otmfsSelection);
}
    ret = IDWriteFont_IsSymbolFont(font);
    ok(!ret, "got %d\n", ret);

    IDWriteFont_Release(font);

    /* weight values */
    for (i = 0; i < sizeof(weights)/(2*sizeof(LONG)); i++)
    {
        memset(&logfont, 0, sizeof(logfont));
        logfont.lfHeight = 12;
        logfont.lfWidth  = 12;
        logfont.lfWeight = weights[i][0];
        lstrcpyW(logfont.lfFaceName, tahomaW);

        hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
        EXPECT_HR(hr, S_OK);

        weight = IDWriteFont_GetWeight(font);
        ok(weight == weights[i][1],
            "%d: got %d, expected %d\n", i, weight, weights[i][1]);
        IDWriteFont_Release(font);
    }

    /* weight not from enum */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = 550;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    weight = IDWriteFont_GetWeight(font);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL || broken(weight == DWRITE_FONT_WEIGHT_BOLD) /* win7 w/o SP */,
        "got %d\n", weight);
    IDWriteFont_Release(font);

    /* empty or nonexistent face name */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    lstrcpyW(logfont.lfFaceName, blahW);

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
todo_wine {
    EXPECT_HR(hr, DWRITE_E_NOFONT);
    ok(font == NULL, "got %p\n", font);
    if(font) IDWriteFont_Release(font);
}

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    lstrcpyW(logfont.lfFaceName, tahomaspW);

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
todo_wine {
    EXPECT_HR(hr, DWRITE_E_NOFONT);
    ok(font == NULL, "got %p\n", font);
    if(font) IDWriteFont_Release(font);
}

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
todo_wine {
    EXPECT_HR(hr, DWRITE_E_NOFONT);
    ok(font == NULL, "got %p\n", font);
    if(font) IDWriteFont_Release(font);
}

    IDWriteGdiInterop_Release(interop);
}

static void test_CreateBitmapRenderTarget(void)
{
    IDWriteBitmapRenderTarget *target, *target2;
    IDWriteGdiInterop *interop;
    HBITMAP hbm, hbm2;
    DWRITE_MATRIX m;
    DIBSECTION ds;
    HRESULT hr;
    SIZE size;
    HDC hdc;
    int ret;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    target = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 0, 0, &target);
    EXPECT_HR(hr, S_OK);

if (0) /* crashes on native */
    hr = IDWriteBitmapRenderTarget_GetSize(target, NULL);

    size.cx = size.cy = -1;
    hr = IDWriteBitmapRenderTarget_GetSize(target, &size);
    EXPECT_HR(hr, S_OK);
    ok(size.cx == 0, "got %d\n", size.cx);
    ok(size.cy == 0, "got %d\n", size.cy);

    target2 = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 0, 0, &target2);
    EXPECT_HR(hr, S_OK);
    ok(target != target2, "got %p, %p\n", target2, target);
    IDWriteBitmapRenderTarget_Release(target2);

    hdc = IDWriteBitmapRenderTarget_GetMemoryDC(target);
    ok(hdc != NULL, "got %p\n", hdc);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm != NULL, "got %p\n", hbm);

    /* check DIB properties */
    ret = GetObjectW(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(BITMAP), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 1, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 1, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(!ds.dsBm.bmBits, "got %p\n", ds.dsBm.bmBits);

    IDWriteBitmapRenderTarget_Release(target);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(!hbm, "got %p\n", hbm);

    target = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 10, 5, &target);
    EXPECT_HR(hr, S_OK);

    hdc = IDWriteBitmapRenderTarget_GetMemoryDC(target);
    ok(hdc != NULL, "got %p\n", hdc);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm != NULL, "got %p\n", hbm);

    /* check DIB properties */
    ret = GetObjectW(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 10, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 5, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 32, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(ds.dsBm.bmBits != NULL, "got %p\n", ds.dsBm.bmBits);

    size.cx = size.cy = -1;
    hr = IDWriteBitmapRenderTarget_GetSize(target, &size);
    EXPECT_HR(hr, S_OK);
    ok(size.cx == 10, "got %d\n", size.cx);
    ok(size.cy == 5, "got %d\n", size.cy);

    /* resize to same size */
    hr = IDWriteBitmapRenderTarget_Resize(target, 10, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 == hbm, "got %p, %p\n", hbm2, hbm);

    /* shrink */
    hr = IDWriteBitmapRenderTarget_Resize(target, 5, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    hr = IDWriteBitmapRenderTarget_Resize(target, 20, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    hr = IDWriteBitmapRenderTarget_Resize(target, 1, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    ret = GetObjectW(hbm2, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 5, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 32, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(ds.dsBm.bmBits != NULL, "got %p\n", ds.dsBm.bmBits);

    /* empty rectangle */
    hr = IDWriteBitmapRenderTarget_Resize(target, 0, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    ret = GetObjectW(hbm2, sizeof(ds), &ds);
    ok(ret == sizeof(BITMAP), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 1, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 1, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(!ds.dsBm.bmBits, "got %p\n", ds.dsBm.bmBits);

    /* transform tests */
if (0) /* crashes on native */
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, NULL);

    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, &m);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(m.m11 == 1.0 && m.m22 == 1.0 && m.m12 == 0.0 && m.m21 == 0.0, "got %.1f,%.1f,%.1f,%.1f\n", m.m11, m.m22, m.m12, m.m21);
    ok(m.dx == 0.0 && m.dy == 0.0, "got %.1f,%.1f\n", m.dx, m.dy);

    IDWriteBitmapRenderTarget_Release(target);
    IDWriteGdiInterop_Release(interop);
}

static void test_GetFontFamily(void)
{
    IDWriteFontFamily *family, *family2;
    IDWriteGdiInterop *interop;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

if (0) /* crashes on native */
    hr = IDWriteFont_GetFontFamily(font, NULL);

    EXPECT_REF(font, 1);
    hr = IDWriteFont_GetFontFamily(font, &family);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(font, 1);
    EXPECT_REF(family, 2);

    hr = IDWriteFont_GetFontFamily(font, &family2);
    EXPECT_HR(hr, S_OK);
    ok(family2 == family, "got %p, previous %p\n", family2, family);
    EXPECT_REF(font, 1);
    EXPECT_REF(family, 3);
    IDWriteFontFamily_Release(family2);

    hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFontFamily, (void**)&family2);
    EXPECT_HR(hr, E_NOINTERFACE);
    ok(family2 == NULL, "got %p\n", family2);

    IDWriteFontFamily_Release(family);
    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
}

static void test_GetFamilyNames(void)
{
    IDWriteFontFamily *family;
    IDWriteLocalizedStrings *names, *names2;
    IDWriteGdiInterop *interop;
    IDWriteFont *font;
    LOGFONTW logfont;
    WCHAR buffer[100];
    HRESULT hr;
    UINT32 len;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    hr = IDWriteFont_GetFontFamily(font, &family);
    EXPECT_HR(hr, S_OK);

if (0) /* crashes on native */
    hr = IDWriteFontFamily_GetFamilyNames(family, NULL);

    hr = IDWriteFontFamily_GetFamilyNames(family, &names);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(names, 1);

    hr = IDWriteFontFamily_GetFamilyNames(family, &names2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(names2, 1);
    ok(names != names2, "got %p, was %p\n", names2, names);

    IDWriteLocalizedStrings_Release(names2);

    /* GetStringLength */
if (0) /* crashes on native */
    hr = IDWriteLocalizedStrings_GetStringLength(names, 0, NULL);

    len = 100;
    hr = IDWriteLocalizedStrings_GetStringLength(names, 10, &len);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(len == (UINT32)-1, "got %u\n", len);

    len = 0;
    hr = IDWriteLocalizedStrings_GetStringLength(names, 0, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len > 0, "got %u\n", len);

    /* GetString */
    hr = IDWriteLocalizedStrings_GetString(names, 0, NULL, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);

    hr = IDWriteLocalizedStrings_GetString(names, 10, NULL, 0);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

if (0)
    hr = IDWriteLocalizedStrings_GetString(names, 0, NULL, 100);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 10, buffer, 100);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len-1);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 0;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(buffer[0] != 0, "got %x\n", buffer[0]);

    IDWriteLocalizedStrings_Release(names);

    IDWriteFontFamily_Release(family);
    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
}

static void test_CreateFontFace(void)
{
    IDWriteFontFace *fontface, *fontface2;
    IDWriteGdiInterop *interop;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFontFace, (void**)&fontface);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDWriteFont_CreateFontFace(font, NULL);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface == fontface2, "got %p, was %p\n", fontface2, fontface);
    /* the fontface refcount is increased here */

    IDWriteFontFace_Release(fontface);

    IDWriteFontFace_Release(fontface);
    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
}

static void test_GetMetrics(void)
{
    IDWriteGdiInterop *interop;
    DWRITE_FONT_METRICS metrics;
    OUTLINETEXTMETRICW otm;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;
    HDC hdc;
    HFONT hfont;
    int ret;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hfont = CreateFontIndirectW(&logfont);
    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    ok(ret, "got %d\n", ret);
    DeleteDC(hdc);
    DeleteObject(hfont);

if (0) /* crashes on native */
    IDWriteFont_GetMetrics(font, NULL);

    memset(&metrics, 0, sizeof(metrics));
    IDWriteFont_GetMetrics(font, &metrics);

    ok(metrics.designUnitsPerEm != 0, "designUnitsPerEm %u\n", metrics.designUnitsPerEm);
    ok(metrics.ascent != 0, "ascent %u\n", metrics.ascent);
    ok(metrics.descent != 0, "descent %u\n", metrics.descent);
todo_wine
    ok(metrics.lineGap == 0, "lineGap %d\n", metrics.lineGap);
    ok(metrics.capHeight, "capHeight %u\n", metrics.capHeight);
    ok(metrics.xHeight != 0, "xHeight %u\n", metrics.xHeight);
    ok(metrics.underlinePosition < 0, "underlinePosition %d\n", metrics.underlinePosition);
    ok(metrics.underlineThickness != 0, "underlineThickness %u\n", metrics.underlineThickness);
    ok(metrics.strikethroughPosition > 0, "strikethroughPosition %d\n", metrics.strikethroughPosition);
    ok(metrics.strikethroughThickness != 0, "strikethroughThickness %u\n", metrics.strikethroughThickness);

    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
}

static void test_system_fontcollection(void)
{
    IDWriteFontCollection *collection, *coll2;
    IDWriteFontFamily *family;
    HRESULT hr;
    UINT32 i;
    BOOL ret;

    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &coll2, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll2 == collection, "got %p, was %p\n", coll2, collection);
    IDWriteFontCollection_Release(coll2);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &coll2, TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll2 == collection, "got %p, was %p\n", coll2, collection);
    IDWriteFontCollection_Release(coll2);

    i = IDWriteFontCollection_GetFontFamilyCount(collection);
    ok(i, "got %u\n", i);

    /* invalid index */
    family = (void*)0xdeadbeef;
    hr = IDWriteFontCollection_GetFontFamily(collection, i, &family);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(family == NULL, "got %p\n", family);

    ret = FALSE;
    i = (UINT32)-1;
    hr = IDWriteFontCollection_FindFamilyName(collection, tahomaW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret, "got %d\n", ret);
    ok(i != (UINT32)-1, "got %u\n", i);

    ret = TRUE;
    i = 0;
    hr = IDWriteFontCollection_FindFamilyName(collection, blahW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!ret, "got %d\n", ret);
    ok(i == (UINT32)-1, "got %u\n", i);

    IDWriteFontCollection_Release(collection);
}

static void test_ConvertFontFaceToLOGFONT(void)
{
    IDWriteGdiInterop *interop;
    IDWriteFontFace *fontface;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfEscapement = 100;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    logfont.lfUnderline = 1;
    logfont.lfStrikeOut = 1;

    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFont_Release(font);

if (0) /* crashes on native */
{
    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, NULL, NULL);
    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, fontface, NULL);
}

    memset(&logfont, 0xa, sizeof(logfont));
    logfont.lfFaceName[0] = 0;

    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, fontface, &logfont);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(logfont.lfHeight == 0, "got %d\n", logfont.lfHeight);
    ok(logfont.lfWidth == 0, "got %d\n", logfont.lfWidth);
    ok(logfont.lfWeight == FW_NORMAL, "got %d\n", logfont.lfWeight);
    ok(logfont.lfEscapement == 0, "got %d\n", logfont.lfEscapement);
    ok(logfont.lfItalic == 1, "got %d\n", logfont.lfItalic);
    ok(logfont.lfUnderline == 0, "got %d\n", logfont.lfUnderline);
    ok(logfont.lfStrikeOut == 0, "got %d\n", logfont.lfStrikeOut);
    ok(!lstrcmpW(logfont.lfFaceName, tahomaW), "got %s\n", wine_dbgstr_w(logfont.lfFaceName));

    IDWriteGdiInterop_Release(interop);
    IDWriteFontFace_Release(fontface);
}

static HRESULT WINAPI fontcollectionloader_QueryInterface(IDWriteFontCollectionLoader *iface, REFIID riid, void **obj)
{
    *obj = iface;
    return S_OK;
}

static ULONG WINAPI fontcollectionloader_AddRef(IDWriteFontCollectionLoader *iface)
{
    return 2;
}

static ULONG WINAPI fontcollectionloader_Release(IDWriteFontCollectionLoader *iface)
{
    return 1;
}

static HRESULT WINAPI fontcollectionloader_CreateEnumeratorFromKey(IDWriteFontCollectionLoader *iface, IDWriteFactory * factory, const void * collectionKey, UINT32  collectionKeySize, IDWriteFontFileEnumerator ** fontFileEnumerator)
{
    return S_OK;
}

static const struct IDWriteFontCollectionLoaderVtbl dwritefontcollectionloadervtbl = {
    fontcollectionloader_QueryInterface,
    fontcollectionloader_AddRef,
    fontcollectionloader_Release,
    fontcollectionloader_CreateEnumeratorFromKey
};

static void test_CustomFontCollection(void)
{
    IDWriteFontCollectionLoader collection = { &dwritefontcollectionloadervtbl };
    IDWriteFontCollectionLoader collection2 = { &dwritefontcollectionloadervtbl };
    HRESULT hr;

    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection);
    ok(hr == DWRITE_E_ALREADYREGISTERED, "got 0x%08x\n", hr);

    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
}

START_TEST(font)
{
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr != S_OK)
    {
        win_skip("failed to create factory\n");
        return;
    }

    test_CreateFontFromLOGFONT();
    test_CreateBitmapRenderTarget();
    test_GetFontFamily();
    test_GetFamilyNames();
    test_CreateFontFace();
    test_GetMetrics();
    test_system_fontcollection();
    test_ConvertFontFaceToLOGFONT();
    test_CustomFontCollection();

    IDWriteFactory_Release(factory);
}
