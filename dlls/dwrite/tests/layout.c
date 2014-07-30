/*
 *    Text layout/format tests
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

static IDWriteFactory *factory;

static const WCHAR tahomaW[] = {'T','a','h','o','m','a',0};
static const WCHAR enusW[] = {'e','n','-','u','s',0};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

static void test_CreateTextLayout(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    IDWriteTextLayout *layout;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextLayout(factory, NULL, 0, NULL, 0.0, 0.0, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, NULL, 0.0, 0.0, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, NULL, 1.0, 0.0, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, NULL, 0.0, 1.0, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, NULL, 1000.0, 1000.0, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

static void test_CreateGdiCompatibleTextLayout(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    FLOAT dimension;
    HRESULT hr;

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, NULL, 0, NULL, 0.0, 0.0, 0.0, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, NULL, 0.0, 0.0, 0.0, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, NULL, 1.0, 0.0, 0.0, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, NULL, 1.0, 0.0, 1.0, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, NULL, 1000.0, 1000.0, 1.0, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* create with text format */
    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(format, 1);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(format, 1);
    EXPECT_REF(layout, 1);

    IDWriteTextLayout_AddRef(layout);
    EXPECT_REF(format, 1);
    EXPECT_REF(layout, 2);
    IDWriteTextLayout_Release(layout);
    IDWriteTextLayout_Release(layout);

    /* zero length string is okay */
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 0, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    dimension = IDWriteTextLayout_GetMaxWidth(layout);
    ok(dimension == 100.0, "got %f\n", dimension);

    dimension = IDWriteTextLayout_GetMaxHeight(layout);
    ok(dimension == 100.0, "got %f\n", dimension);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
}

static void test_CreateTextFormat(void)
{
    IDWriteFontCollection *collection, *syscoll;
    DWRITE_PARAGRAPH_ALIGNMENT paralign;
    DWRITE_READING_DIRECTION readdir;
    DWRITE_WORD_WRAPPING wrapping;
    DWRITE_TEXT_ALIGNMENT align;
    DWRITE_FLOW_DIRECTION flow;
    DWRITE_LINE_SPACING_METHOD method;
    DWRITE_TRIMMING trimming;
    IDWriteTextFormat *format;
    FLOAT spacing, baseline;
    IDWriteInlineObject *trimmingsign;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDWriteTextFormat_GetFontCollection(format, NULL);

    collection = NULL;
    hr = IDWriteTextFormat_GetFontCollection(format, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(collection != NULL, "got %p\n", collection);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscoll, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(collection == syscoll, "got %p, was %p\n", syscoll, collection);
    IDWriteFontCollection_Release(syscoll);
    IDWriteFontCollection_Release(collection);

    /* default format properties */
    align = IDWriteTextFormat_GetTextAlignment(format);
    ok(align == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", align);

    paralign = IDWriteTextFormat_GetParagraphAlignment(format);
    ok(paralign == DWRITE_PARAGRAPH_ALIGNMENT_NEAR, "got %d\n", paralign);

    wrapping = IDWriteTextFormat_GetWordWrapping(format);
    ok(wrapping == DWRITE_WORD_WRAPPING_WRAP, "got %d\n", wrapping);

    readdir = IDWriteTextFormat_GetReadingDirection(format);
    ok(readdir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", readdir);

    flow = IDWriteTextFormat_GetFlowDirection(format);
    ok(flow == DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM, "got %d\n", flow);

    hr = IDWriteTextFormat_GetLineSpacing(format, &method, &spacing, &baseline);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(spacing == 0.0, "got %f\n", spacing);
    ok(baseline == 0.0, "got %f\n", baseline);
    ok(method == DWRITE_LINE_SPACING_METHOD_DEFAULT, "got %d\n", method);

    trimming.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;
    trimming.delimiter = 10;
    trimming.delimiterCount = 10;
    trimmingsign = (void*)0xdeadbeef;
    hr = IDWriteTextFormat_GetTrimming(format, &trimming, &trimmingsign);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(trimming.granularity == DWRITE_TRIMMING_GRANULARITY_NONE, "got %d\n", trimming.granularity);
    ok(trimming.delimiter == 0, "got %d\n", trimming.delimiter);
    ok(trimming.delimiterCount == 0, "got %d\n", trimming.delimiterCount);
    ok(trimmingsign == NULL, "got %p\n", trimmingsign);

    /* setters */
    hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_LEADING);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_WRAP);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetTrimming(format, &trimming, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteTextFormat_Release(format);
}

static void test_GetLocaleName(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    WCHAR buff[10];
    UINT32 len;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 0, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    len = IDWriteTextLayout_GetLocaleNameLength(layout);
    ok(len == 2, "got %u\n", len);
    len = IDWriteTextFormat_GetLocaleNameLength(format);
    ok(len == 2, "got %u\n", len);
    hr = IDWriteTextLayout_GetLocaleName(layout, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    hr = IDWriteTextLayout_GetLocaleName(layout, buff, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buff, ruW), "got %s\n", wine_dbgstr_w(buff));
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buff, ruW), "got %s\n", wine_dbgstr_w(buff));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
}

static void test_CreateEllipsisTrimmingSign(void)
{
    DWRITE_BREAK_CONDITION before, after;
    IDWriteTextFormat *format;
    IDWriteInlineObject *sign;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(format, 1);
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(format, 1);

if (0) /* crashes on native */
    hr = IDWriteInlineObject_GetBreakConditions(sign, NULL, NULL);

    before = after = DWRITE_BREAK_CONDITION_CAN_BREAK;
    hr = IDWriteInlineObject_GetBreakConditions(sign, &before, &after);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(before == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", before);
    ok(after == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", after);

    IDWriteInlineObject_Release(sign);
    IDWriteTextFormat_Release(format);
}

static void test_fontweight(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};
    IDWriteTextFormat *format, *fmt2;
    IDWriteTextLayout *layout;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_TEXT_RANGE range;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 0, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&fmt2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    weight = IDWriteTextFormat_GetFontWeight(fmt2);
todo_wine
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.startPosition = 0;
    range.length = 6;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* IDWriteTextFormat methods output doesn't reflect layout changes */
    weight = IDWriteTextFormat_GetFontWeight(fmt2);
todo_wine
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.length = 0;
    weight = DWRITE_FONT_WEIGHT_BOLD;
    hr = layout->lpVtbl->IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);
    ok(range.length == 6, "got %d\n", range.length);
}
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(fmt2);
    IDWriteTextFormat_Release(format);
}

static void test_SetInlineObject(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};

    IDWriteInlineObject *inlineobj, *inlineobj2;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 0, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj2, range);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 1;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
}

START_TEST(layout)
{
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr != S_OK)
    {
        win_skip("failed to create factory\n");
        return;
    }

    test_CreateTextLayout();
    test_CreateGdiCompatibleTextLayout();
    test_CreateTextFormat();
    test_GetLocaleName();
    test_CreateEllipsisTrimmingSign();
    test_fontweight();
    test_SetInlineObject();

    IDWriteFactory_Release(factory);
}
