/*
 *    Text layout/format tests
 *
 * Copyright 2012, 2014-2015 Nikolay Sivov for CodeWeavers
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

#include <assert.h>

#include "windows.h"
#include "dwrite.h"
#include "dwrite_2.h"

#include "wine/test.h"

static const WCHAR tahomaW[] = {'T','a','h','o','m','a',0};
static const WCHAR enusW[] = {'e','n','-','u','s',0};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

static IDWriteFactory *create_factory(void)
{
    IDWriteFactory *factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    return factory;
}

enum drawcall_kind {
    DRAW_GLYPHRUN = 0,
    DRAW_UNDERLINE,
    DRAW_STRIKETHROUGH,
    DRAW_INLINE,
    DRAW_LAST_KIND
};

static const char *get_draw_kind_name(enum drawcall_kind kind)
{
    static const char *kind_names[] = { "GLYPH_RUN", "UNDERLINE", "STRIKETHROUGH", "INLINE", "END_OF_SEQ" };
    return kind > DRAW_LAST_KIND ? "unknown" : kind_names[kind];
}

struct drawcall_entry {
    enum drawcall_kind kind;
};

struct drawcall_sequence
{
    int count;
    int size;
    struct drawcall_entry *sequence;
};

struct drawtestcontext {
    enum drawcall_kind kind;
    BOOL todo;
    int *failcount;
    const char *file;
    int line;
};

#define NUM_CALL_SEQUENCES 1
#define RENDERER_ID        0
static struct drawcall_sequence *sequences[NUM_CALL_SEQUENCES];
static struct drawcall_sequence *expected_seq[1];

static void add_call(struct drawcall_sequence **seq, int sequence_index, const struct drawcall_entry *call)
{
    struct drawcall_sequence *call_seq = seq[sequence_index];

    if (!call_seq->sequence) {
        call_seq->size = 10;
        call_seq->sequence = HeapAlloc(GetProcessHeap(), 0, call_seq->size * sizeof (struct drawcall_entry));
    }

    if (call_seq->count == call_seq->size) {
        call_seq->size *= 2;
        call_seq->sequence = HeapReAlloc(GetProcessHeap(), 0,
                                        call_seq->sequence,
                                        call_seq->size * sizeof (struct drawcall_entry));
    }

    assert(call_seq->sequence);
    call_seq->sequence[call_seq->count++] = *call;
}

static inline void flush_sequence(struct drawcall_sequence **seg, int sequence_index)
{
    struct drawcall_sequence *call_seq = seg[sequence_index];

    HeapFree(GetProcessHeap(), 0, call_seq->sequence);
    call_seq->sequence = NULL;
    call_seq->count = call_seq->size = 0;
}

static inline void flush_sequences(struct drawcall_sequence **seq, int n)
{
    int i;
    for (i = 0; i < n; i++)
        flush_sequence(seq, i);
}

static void init_call_sequences(struct drawcall_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct drawcall_sequence));
}

static void ok_sequence_(struct drawcall_sequence **seq, int sequence_index,
    const struct drawcall_entry *expected, const char *context, BOOL todo,
    const char *file, int line)
{
    static const struct drawcall_entry end_of_sequence = { DRAW_LAST_KIND };
    struct drawcall_sequence *call_seq = seq[sequence_index];
    const struct drawcall_entry *actual, *sequence;
    int failcount = 0;

    add_call(seq, sequence_index, &end_of_sequence);

    sequence = call_seq->sequence;
    actual = sequence;

    while (expected->kind != DRAW_LAST_KIND && actual->kind != DRAW_LAST_KIND) {
        if (expected->kind != actual->kind) {
            if (todo) {
                failcount++;
                todo_wine
                    ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                        context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));

                flush_sequence(seq, sequence_index);
                return;
            }
            else
                ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                    context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));
        }
        expected++;
        actual++;
    }

    if (todo) {
        todo_wine {
            if (expected->kind != DRAW_LAST_KIND || actual->kind != DRAW_LAST_KIND) {
                failcount++;
                ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
                    context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));
            }
        }
    }
    else if (expected->kind != DRAW_LAST_KIND || actual->kind != DRAW_LAST_KIND)
        ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
            context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));

    if (todo && !failcount) /* succeeded yet marked todo */
        todo_wine
            ok_(file, line)(1, "%s: marked \"todo_wine\" but succeeds\n", context);

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)

static HRESULT WINAPI testrenderer_QI(IDWriteTextRenderer *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextRenderer) ||
        IsEqualIID(riid, &IID_IDWritePixelSnapping) ||
        IsEqualIID(riid, &IID_IUnknown)
    ) {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;

    /* IDWriteTextRenderer1 overrides drawing calls, ignore for now */
    if (IsEqualIID(riid, &IID_IDWriteTextRenderer1))
        return E_NOINTERFACE;

    ok(0, "unexpected QI %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI testrenderer_AddRef(IDWriteTextRenderer *iface)
{
    return 2;
}

static ULONG WINAPI testrenderer_Release(IDWriteTextRenderer *iface)
{
    return 1;
}

static HRESULT WINAPI testrenderer_IsPixelSnappingDisabled(IDWriteTextRenderer *iface,
    void *client_drawingcontext, BOOL *disabled)
{
    *disabled = TRUE;
    return S_OK;
}

static HRESULT WINAPI testrenderer_GetCurrentTransform(IDWriteTextRenderer *iface,
    void *client_drawingcontext, DWRITE_MATRIX *transform)
{
    transform->m11 = 1.0;
    transform->m12 = 0.0;
    transform->m21 = 0.0;
    transform->m22 = 1.0;
    transform->dx = 0.0;
    transform->dy = 0.0;
    return S_OK;
}

static HRESULT WINAPI testrenderer_GetPixelsPerDip(IDWriteTextRenderer *iface,
    void *client_drawingcontext, FLOAT *pixels_per_dip)
{
    *pixels_per_dip = 1.0;
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawGlyphRun(IDWriteTextRenderer *iface,
    void* client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE mode,
    DWRITE_GLYPH_RUN const *glyph_run,
    DWRITE_GLYPH_RUN_DESCRIPTION const *run_descr,
    IUnknown *drawing_effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_GLYPHRUN;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawUnderline(IDWriteTextRenderer *iface,
    void *client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_UNDERLINE const* underline,
    IUnknown *drawing_effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_UNDERLINE;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawStrikethrough(IDWriteTextRenderer *iface,
    void *client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown *drawing_effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_STRIKETHROUGH;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawInlineObject(IDWriteTextRenderer *iface,
    void *client_drawingcontext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject *object,
    BOOL is_sideways,
    BOOL is_rtl,
    IUnknown *drawing_effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_INLINE;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static const IDWriteTextRendererVtbl testrenderervtbl = {
    testrenderer_QI,
    testrenderer_AddRef,
    testrenderer_Release,
    testrenderer_IsPixelSnappingDisabled,
    testrenderer_GetCurrentTransform,
    testrenderer_GetPixelsPerDip,
    testrenderer_DrawGlyphRun,
    testrenderer_DrawUnderline,
    testrenderer_DrawStrikethrough,
    testrenderer_DrawInlineObject
};

static IDWriteTextRenderer testrenderer = { &testrenderervtbl };

static void test_CreateTextLayout(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    IDWriteTextLayout2 *layout2;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

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

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(format, 1);
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(format, 1);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    if (hr == S_OK) {
        IDWriteTextLayout1 *layout1;
        IDWriteTextFormat1 *format1;
        IDWriteTextFormat *format;

        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextLayout1, (void**)&layout1);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IDWriteTextLayout1_Release(layout1);

        EXPECT_REF(layout2, 2);
        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void**)&format1);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        EXPECT_REF(layout2, 3);

        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat, (void**)&format);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(format == (IDWriteTextFormat*)format1, "got %p, %p\n", format, format1);
        ok(format != (IDWriteTextFormat*)layout2, "got %p, %p\n", format, layout2);
        EXPECT_REF(layout2, 4);

        hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextLayout1, (void**)&layout1);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IDWriteTextLayout1_Release(layout1);

        IDWriteTextFormat1_Release(format1);
        IDWriteTextFormat_Release(format);

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat1, (void**)&format1);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        EXPECT_REF(layout2, 3);

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&format);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(format == (IDWriteTextFormat*)format1, "got %p, %p\n", format, format1);
        EXPECT_REF(layout2, 4);

        IDWriteTextFormat1_Release(format1);
        IDWriteTextFormat_Release(format);
        IDWriteTextLayout2_Release(layout2);
    }
    else
        win_skip("IDWriteTextLayout2 is not supported.\n");

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_CreateGdiCompatibleTextLayout(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    FLOAT dimension;
    HRESULT hr;

    factory = create_factory();

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
    IDWriteFactory_Release(factory);
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
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

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
    IDWriteFactory_Release(factory);
}

static void test_GetLocaleName(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format, *format2;
    IDWriteFactory *factory;
    WCHAR buff[10];
    UINT32 len;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 0, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&format2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    len = IDWriteTextFormat_GetLocaleNameLength(format2);
    ok(len == 2, "got %u\n", len);
    len = IDWriteTextFormat_GetLocaleNameLength(format);
    ok(len == 2, "got %u\n", len);
    hr = IDWriteTextFormat_GetLocaleName(format2, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    hr = IDWriteTextFormat_GetLocaleName(format2, buff, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buff, ruW), "got %s\n", wine_dbgstr_w(buff));
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buff, ruW), "got %s\n", wine_dbgstr_w(buff));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteTextFormat_Release(format2);
    IDWriteFactory_Release(factory);
}

static void test_CreateEllipsisTrimmingSign(void)
{
    DWRITE_BREAK_CONDITION before, after;
    IDWriteTextFormat *format;
    IDWriteInlineObject *sign;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

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
    IDWriteFactory_Release(factory);
}

static void test_fontweight(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};
    IDWriteTextFormat *format, *fmt2;
    IDWriteTextLayout *layout;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    FLOAT size;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&fmt2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    weight = IDWriteTextFormat_GetFontWeight(fmt2);
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.startPosition = 0;
    range.length = 6;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* IDWriteTextFormat methods output doesn't reflect layout changes */
    weight = IDWriteTextFormat_GetFontWeight(fmt2);
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.length = 0;
    weight = DWRITE_FONT_WEIGHT_BOLD;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);
    ok(range.length == 6, "got %d\n", range.length);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 100.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, -1.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, 100.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 100.0, "got %.2f\n", size);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 100.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, -1.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, 100.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 100.0, "got %.2f\n", size);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(fmt2);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetInlineObject(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};

    IDWriteInlineObject *inlineobj, *inlineobj2, *inlinetest;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range, r2;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(inlineobj, 1);
    EXPECT_REF(inlineobj2, 1);

    inlinetest = (void*)0x1;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == NULL, "got %p\n", inlinetest);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(inlineobj, 2);

    inlinetest = (void*)0x1;
    hr = IDWriteTextLayout_GetInlineObject(layout, 2, &inlinetest, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == NULL, "got %p\n", inlinetest);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    /* get from somewhere inside a range */
    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 1, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj2, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 1, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj2, "got %p\n", inlinetest);
    ok(r2.startPosition == 1 && r2.length == 1, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);
    EXPECT_REF(inlineobj2, 2);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 1, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(inlineobj, 2);

    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 3, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);
    EXPECT_REF(inlineobj2, 1);

    IDWriteTextLayout_Release(layout);

    EXPECT_REF(inlineobj, 1);

    IDWriteInlineObject_Release(inlineobj);
    IDWriteInlineObject_Release(inlineobj2);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

/* drawing calls sequence doesn't depend on run order, instead all runs are
   drawn first, inline objects next and then underline/strikes */
static const struct drawcall_entry draw_seq[] = {
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_INLINE },
    { DRAW_UNDERLINE },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq2[] = {
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN },
    { DRAW_LAST_KIND }
};

static void test_Draw(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR ruW[] = {'r','u',0};

    IDWriteInlineObject *inlineobj;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 5;
    range.length = 1;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 4;
    range.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, (IUnknown*)inlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq, "draw test", TRUE);
    IDWriteTextLayout_Release(layout);

    /* with reduced width DrawGlyphRun() is called for every line */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 5.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq2, "draw test 2", TRUE);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_typography(void)
{
    DWRITE_FONT_FEATURE feature;
    IDWriteTypography *typography;
    IDWriteFactory *factory;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTypography(factory, &typography);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    feature.nameTag = DWRITE_FONT_FEATURE_TAG_KERNING;
    feature.parameter = 1;
    hr = IDWriteTypography_AddFontFeature(typography, feature);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = IDWriteTypography_GetFontFeatureCount(typography);
    ok(count == 1, "got %u\n", count);

    /* duplicated features work just fine */
    feature.nameTag = DWRITE_FONT_FEATURE_TAG_KERNING;
    feature.parameter = 0;
    hr = IDWriteTypography_AddFontFeature(typography, feature);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = IDWriteTypography_GetFontFeatureCount(typography);
    ok(count == 2, "got %u\n", count);

    memset(&feature, 0, sizeof(feature));
    hr = IDWriteTypography_GetFontFeature(typography, 0, &feature);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(feature.nameTag == DWRITE_FONT_FEATURE_TAG_KERNING, "got tag %x\n", feature.nameTag);
    ok(feature.parameter == 1, "got %u\n", feature.parameter);

    memset(&feature, 0, sizeof(feature));
    hr = IDWriteTypography_GetFontFeature(typography, 1, &feature);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(feature.nameTag == DWRITE_FONT_FEATURE_TAG_KERNING, "got tag %x\n", feature.nameTag);
    ok(feature.parameter == 0, "got %u\n", feature.parameter);

    hr = IDWriteTypography_GetFontFeature(typography, 2, &feature);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    IDWriteTypography_Release(typography);
    IDWriteFactory_Release(factory);
}

static void test_GetClusterMetrics(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    DWRITE_INLINE_OBJECT_METRICS inline_metrics;
    DWRITE_CLUSTER_METRICS metrics;
    IDWriteInlineObject *trimm;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count == 4, "got %u\n", count);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &trimm);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, trimm, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* inline object takes a separate cluster, replaced codepoints number doesn't matter */
    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
todo_wine
    ok(count == 3, "got %u\n", count);

    count = 0;
    memset(&metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, &metrics, 1, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
todo_wine
    ok(count == 3, "got %u\n", count);
    ok(metrics.length == 2, "got %u\n", metrics.length);

    hr = IDWriteInlineObject_GetMetrics(trimm, &inline_metrics);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(inline_metrics.width == metrics.width, "got %.2f, expected %.2f\n", inline_metrics.width,
        metrics.width);
}
    IDWriteInlineObject_Release(trimm);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetLocaleName(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    WCHAR buffW[LOCALE_NAME_MAX_LENGTH+sizeof(strW)/sizeof(WCHAR)];
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetLocaleName(layout, enusW, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_SetLocaleName(layout, NULL, range);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* invalid locale name is allowed */
    hr = IDWriteTextLayout_SetLocaleName(layout, strW, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_GetLocaleName(layout, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, NULL, 1, NULL);

    buffW[0] = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, sizeof(buffW)/sizeof(WCHAR), NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, strW), "got %s\n", wine_dbgstr_w(buffW));

    /* get with a shorter buffer */
    buffW[0] = 0xa;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, 1, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(buffW[0] == 0, "got %x\n", buffW[0]);

    /* name is too long */
    lstrcpyW(buffW, strW);
    while (lstrlenW(buffW) <= LOCALE_NAME_MAX_LENGTH)
        lstrcatW(buffW, strW);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetLocaleName(layout, buffW, range);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    buffW[0] = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, sizeof(buffW)/sizeof(WCHAR), NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, strW), "got %s\n", wine_dbgstr_w(buffW));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetPairKerning(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextLayout1 *layout1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    BOOL kerning;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout1, (void**)&layout1);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("SetPairKerning() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

if (0) { /* crashes on native */
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, NULL, NULL);
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, NULL, &range);
}

    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 0;
    kerning = TRUE;
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, &range);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!kerning, "got %d\n", kerning);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout1_SetPairKerning(layout1, 2, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    kerning = FALSE;
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, &range);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(kerning == TRUE, "got %d\n", kerning);

    IDWriteTextLayout1_Release(layout1);
    IDWriteFactory_Release(factory);
}

static void test_SetVerticalGlyphOrientation(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    DWRITE_VERTICAL_GLYPH_ORIENTATION orientation;
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("SetVerticalGlyphOrientation() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

    orientation = IDWriteTextLayout2_GetVerticalGlyphOrientation(layout2);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT, "got %d\n", orientation);

    hr = IDWriteTextLayout2_SetVerticalGlyphOrientation(layout2, DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED+1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    IDWriteTextLayout2_Release(layout2);
    IDWriteFactory_Release(factory);
}

static void test_fallback(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteFontFallback *fallback, *fallback2;
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat1 *format1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory2 *factory2;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("GetFontFallback() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

if (0) /* crashes on native */
    hr = IDWriteTextLayout2_GetFontFallback(layout2, NULL);

    fallback = (void*)0xdeadbeef;
    hr = IDWriteTextLayout2_GetFontFallback(layout2, &fallback);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fallback == NULL, "got %p\n", fallback);

    hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void**)&format1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fallback = (void*)0xdeadbeef;
    hr = IDWriteTextFormat1_GetFontFallback(format1, &fallback);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fallback == NULL, "got %p\n", fallback);

    hr = IDWriteFactory_QueryInterface(factory, &IID_IDWriteFactory2, (void**)&factory2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fallback = NULL;
    hr = IDWriteFactory2_GetSystemFontFallback(factory2, &fallback);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);
if (hr == S_OK) {
    ok(fallback != NULL, "got %p\n", fallback);

    hr = IDWriteTextFormat1_SetFontFallback(format1, fallback);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fallback2 = (void*)0xdeadbeef;
    hr = IDWriteTextLayout2_GetFontFallback(layout2, &fallback2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fallback2 == fallback, "got %p\n", fallback2);

    hr = IDWriteTextLayout2_SetFontFallback(layout2, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fallback2 = (void*)0xdeadbeef;
    hr = IDWriteTextFormat1_GetFontFallback(format1, &fallback2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fallback2 == NULL, "got %p\n", fallback2);

    IDWriteFontFallback_Release(fallback);
}
    IDWriteTextFormat1_Release(format1);
    IDWriteTextLayout2_Release(layout2);
    IDWriteFactory_Release(factory);
}

static void test_DetermineMinWidth(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    FLOAT minwidth;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, lstrlenW(strW), format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_DetermineMinWidth(layout, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    minwidth = 0.0;
    hr = IDWriteTextLayout_DetermineMinWidth(layout, &minwidth);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(minwidth > 0.0, "got %.2f\n", minwidth);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
}

START_TEST(layout)
{
    IDWriteFactory *factory;

    if (!(factory = create_factory())) {
        win_skip("failed to create factory\n");
        return;
    }

    init_call_sequences(sequences, NUM_CALL_SEQUENCES);
    init_call_sequences(expected_seq, 1);

    test_CreateTextLayout();
    test_CreateGdiCompatibleTextLayout();
    test_CreateTextFormat();
    test_GetLocaleName();
    test_CreateEllipsisTrimmingSign();
    test_fontweight();
    test_SetInlineObject();
    test_Draw();
    test_typography();
    test_GetClusterMetrics();
    test_SetLocaleName();
    test_SetPairKerning();
    test_SetVerticalGlyphOrientation();
    test_fallback();
    test_DetermineMinWidth();

    IDWriteFactory_Release(factory);
}
