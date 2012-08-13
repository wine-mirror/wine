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

#include "initguid.h"
#include "dwrite.h"

#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static IDWriteFactory *factory;

static void test_CreateFontFromLOGFONT(void)
{
    static const WCHAR arialW[] = {'A','r','i','a','l',0};
    static const WCHAR blahW[]  = {'B','l','a','h','!',0};
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
    HRESULT hr;
    BOOL ret;
    int i;

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
    lstrcpyW(logfont.lfFaceName, arialW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    /* now check properties */
    weight = IDWriteFont_GetWeight(font);
todo_wine
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);

    style = IDWriteFont_GetStyle(font);
    ok(style == DWRITE_FONT_STYLE_ITALIC, "got %d\n", style);

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
        lstrcpyW(logfont.lfFaceName, arialW);

        hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
        EXPECT_HR(hr, S_OK);

        weight = IDWriteFont_GetWeight(font);
    todo_wine
        ok(weight == weights[i][1],
            "%d: got %d, expected %d\n", i, weight, weights[i][1]);
        IDWriteFont_Release(font);
    }

    /* weight not from enum */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = 550;
    lstrcpyW(logfont.lfFaceName, arialW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    weight = IDWriteFont_GetWeight(font);
todo_wine
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL || broken(weight == DWRITE_FONT_WEIGHT_BOLD) /* win7 w/o SP */,
        "got %d\n", weight);
    IDWriteFont_Release(font);

    /* empty or nonexistent face name */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    lstrcpyW(logfont.lfFaceName, blahW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
todo_wine
    EXPECT_HR(hr, DWRITE_E_NOFONT);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
todo_wine
    EXPECT_HR(hr, DWRITE_E_NOFONT);

    IDWriteGdiInterop_Release(interop);
}

static void test_CreateBitmapRenderTarget(void)
{
    IDWriteBitmapRenderTarget *target, *target2;
    IDWriteGdiInterop *interop;
    DIBSECTION ds;
    HBITMAP hbm;
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

    IDWriteBitmapRenderTarget_Release(target);

    IDWriteGdiInterop_Release(interop);
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

    IDWriteFactory_Release(factory);
}
