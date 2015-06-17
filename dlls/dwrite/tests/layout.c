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

static DWRITE_SCRIPT_ANALYSIS g_sa;
static DWRITE_SCRIPT_ANALYSIS g_control_sa;

/* test IDWriteTextAnalysisSink */
static HRESULT WINAPI analysissink_QueryInterface(IDWriteTextAnalysisSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSink) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI analysissink_AddRef(IDWriteTextAnalysisSink *iface)
{
    return 2;
}

static ULONG WINAPI analysissink_Release(IDWriteTextAnalysisSink *iface)
{
    return 1;
}

static HRESULT WINAPI analysissink_SetScriptAnalysis(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* sa)
{
    g_sa = *sa;
    return S_OK;
}

static HRESULT WINAPI analysissink_SetLineBreakpoints(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetBidiLevel(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, UINT8 explicitLevel, UINT8 resolvedLevel)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetNumberSubstitution(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, IDWriteNumberSubstitution* substitution)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static IDWriteTextAnalysisSinkVtbl analysissinkvtbl = {
    analysissink_QueryInterface,
    analysissink_AddRef,
    analysissink_Release,
    analysissink_SetScriptAnalysis,
    analysissink_SetLineBreakpoints,
    analysissink_SetBidiLevel,
    analysissink_SetNumberSubstitution
};

static IDWriteTextAnalysisSink analysissink = { &analysissinkvtbl };

/* test IDWriteTextAnalysisSource */
static HRESULT WINAPI analysissource_QueryInterface(IDWriteTextAnalysisSource *iface,
    REFIID riid, void **obj)
{
    ok(0, "QueryInterface not expected\n");
    return E_NOTIMPL;
}

static ULONG WINAPI analysissource_AddRef(IDWriteTextAnalysisSource *iface)
{
    ok(0, "AddRef not expected\n");
    return 2;
}

static ULONG WINAPI analysissource_Release(IDWriteTextAnalysisSource *iface)
{
    ok(0, "Release not expected\n");
    return 1;
}

static const WCHAR *g_source;

static HRESULT WINAPI analysissource_GetTextAtPosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    if (position >= lstrlenW(g_source))
    {
        *text = NULL;
        *text_len = 0;
    }
    else
    {
        *text = &g_source[position];
        *text_len = lstrlenW(g_source) - position;
    }

    return S_OK;
}

static HRESULT WINAPI analysissource_GetTextBeforePosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static DWRITE_READING_DIRECTION WINAPI analysissource_GetParagraphReadingDirection(
    IDWriteTextAnalysisSource *iface)
{
    ok(0, "unexpected\n");
    return DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
}

static HRESULT WINAPI analysissource_GetLocaleName(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, WCHAR const** locale)
{
    *locale = NULL;
    *text_len = 0;
    return S_OK;
}

static HRESULT WINAPI analysissource_GetNumberSubstitution(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, IDWriteNumberSubstitution **substitution)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static IDWriteTextAnalysisSourceVtbl analysissourcevtbl = {
    analysissource_QueryInterface,
    analysissource_AddRef,
    analysissource_Release,
    analysissource_GetTextAtPosition,
    analysissource_GetTextBeforePosition,
    analysissource_GetParagraphReadingDirection,
    analysissource_GetLocaleName,
    analysissource_GetNumberSubstitution
};

static IDWriteTextAnalysisSource analysissource = { &analysissourcevtbl };

static IDWriteFactory *create_factory(void)
{
    IDWriteFactory *factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    return factory;
}

/* obvious limitation is that only last script data is returned, so this
   helper is suitable for single script strings only */
static void get_script_analysis(const WCHAR *str, UINT32 len, DWRITE_SCRIPT_ANALYSIS *sa)
{
    IDWriteTextAnalyzer *analyzer;
    IDWriteFactory *factory;
    HRESULT hr;

    g_source = str;

    factory = create_factory();
    hr = IDWriteFactory_CreateTextAnalyzer(factory, &analyzer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &analysissource, 0, len, &analysissink);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    *sa = g_sa;
    IDWriteFactory_Release(factory);
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc = IUnknown_AddRef(obj);
    IUnknown_Release(obj);
    ok_(__FILE__,line)(rc-1 == ref, "expected refcount %d, got %d\n", ref, rc-1);
}

enum drawcall_modifiers_kind {
    DRAW_EFFECT         = 0x1000
};

enum drawcall_kind {
    DRAW_GLYPHRUN      = 0,
    DRAW_UNDERLINE     = 1,
    DRAW_STRIKETHROUGH = 2,
    DRAW_INLINE        = 3,
    DRAW_LAST_KIND     = 4,
    DRAW_TOTAL_KINDS   = 5,
    DRAW_KINDS_MASK    = 0xff
};

static const char *get_draw_kind_name(unsigned short kind)
{
    static const char *kind_names[] = {
      "GLYPH_RUN",
      "UNDERLINE",
      "STRIKETHROUGH",
      "INLINE",
      "END_OF_SEQ",

      "GLYPH_RUN|EFFECT",
      "UNDERLINE|EFFECT",
      "STRIKETHROUGH|EFFECT",
      "INLINE|EFFECT",
      "END_OF_SEQ"
    };
    if ((kind & DRAW_KINDS_MASK) > DRAW_LAST_KIND)
        return "unknown";
    return (kind & DRAW_EFFECT) ? kind_names[(kind & DRAW_KINDS_MASK) + DRAW_TOTAL_KINDS] :
        kind_names[kind];
}

struct drawcall_entry {
    enum drawcall_kind kind;
    WCHAR string[10]; /* only meaningful for DrawGlyphRun() */
};

struct drawcall_sequence
{
    int count;
    int size;
    struct drawcall_entry *sequence;
};

struct drawtestcontext {
    unsigned short kind;
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
        else if ((expected->kind & DRAW_KINDS_MASK) == DRAW_GLYPHRUN) {
            int cmp = lstrcmpW(expected->string, actual->string);
            if (cmp != 0 && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: glyphrun string %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->string), wine_dbgstr_w(actual->string));
            }
            else
                ok_(file, line) (cmp == 0, "%s: glyphrun string %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->string), wine_dbgstr_w(actual->string));
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testrenderer_GetPixelsPerDip(IDWriteTextRenderer *iface,
    void *client_drawingcontext, FLOAT *pixels_per_dip)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testrenderer_DrawGlyphRun(IDWriteTextRenderer *iface,
    void* client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE mode,
    DWRITE_GLYPH_RUN const *run,
    DWRITE_GLYPH_RUN_DESCRIPTION const *descr,
    IUnknown *effect)
{
    struct drawcall_entry entry;
    DWRITE_SCRIPT_ANALYSIS sa;

    ok(descr->stringLength < sizeof(entry.string)/sizeof(WCHAR), "string is too long\n");
    if (descr->stringLength && descr->stringLength < sizeof(entry.string)/sizeof(WCHAR)) {
        memcpy(entry.string, descr->string, descr->stringLength*sizeof(WCHAR));
        entry.string[descr->stringLength] = 0;
    }
    else
        entry.string[0] = 0;

    /* see what's reported for control codes runs */
    get_script_analysis(descr->string, descr->stringLength, &sa);
    if (sa.script == g_control_sa.script) {
        /* glyphs are not reported at all for control code runs */
        ok(run->glyphCount == 0, "got %u\n", run->glyphCount);
        ok(run->glyphAdvances != NULL, "advances array %p\n", run->glyphAdvances);
        ok(run->glyphOffsets != NULL, "offsets array %p\n", run->glyphOffsets);
        ok(run->fontFace != NULL, "got %p\n", run->fontFace);
        /* text positions are still valid */
        ok(descr->string != NULL, "got string %p\n", descr->string);
        ok(descr->stringLength > 0, "got string length %u\n", descr->stringLength);
        ok(descr->clusterMap != NULL, "clustermap %p\n", descr->clusterMap);
    }

    entry.kind = DRAW_GLYPHRUN;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawUnderline(IDWriteTextRenderer *iface,
    void *client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_UNDERLINE const* underline,
    IUnknown *effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_UNDERLINE;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawStrikethrough(IDWriteTextRenderer *iface,
    void *client_drawingcontext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown *effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_STRIKETHROUGH;
    if (effect)
        entry.kind |= DRAW_EFFECT;
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
    IUnknown *effect)
{
    struct drawcall_entry entry;
    entry.kind = DRAW_INLINE;
    if (effect)
        entry.kind |= DRAW_EFFECT;
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

/* test IDWriteInlineObject */
static HRESULT WINAPI testinlineobj_QI(IDWriteInlineObject *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteInlineObject) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteInlineObject_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testinlineobj_AddRef(IDWriteInlineObject *iface)
{
    return 2;
}

static ULONG WINAPI testinlineobj_Release(IDWriteInlineObject *iface)
{
    return 1;
}

static HRESULT WINAPI testinlineobj_Draw(IDWriteInlineObject *iface,
    void* client_drawingontext, IDWriteTextRenderer* renderer,
    FLOAT originX, FLOAT originY, BOOL is_sideways, BOOL is_rtl, IUnknown *drawing_effect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinlineobj_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *metrics)
{
    metrics->width = 123.0;
    return 0x8faecafe;
}

static HRESULT WINAPI testinlineobj_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinlineobj_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
    DWRITE_BREAK_CONDITION *after)
{
    *before = *after = DWRITE_BREAK_CONDITION_MUST_BREAK;
    return 0x8feacafe;
}

static IDWriteInlineObjectVtbl testinlineobjvtbl = {
    testinlineobj_QI,
    testinlineobj_AddRef,
    testinlineobj_Release,
    testinlineobj_Draw,
    testinlineobj_GetMetrics,
    testinlineobj_GetOverhangMetrics,
    testinlineobj_GetBreakConditions
};

static IDWriteInlineObject testinlineobj = { &testinlineobjvtbl };
static IDWriteInlineObject testinlineobj2 = { &testinlineobjvtbl };

static HRESULT WINAPI testeffect_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testeffect_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI testeffect_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testeffectvtbl = {
    testeffect_QI,
    testeffect_AddRef,
    testeffect_Release
};

static IUnknown testeffect = { &testeffectvtbl };

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

    hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_JUSTIFIED+1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_CENTER+1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_WRAP);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_CHARACTER+1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0, -10.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, -10.0, 0.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_UNIFORM+1, 0.0, 0.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

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

static const struct drawcall_entry drawellipsis_seq[] = {
    { DRAW_GLYPHRUN, {0x2026, 0} },
    { DRAW_LAST_KIND }
};

static void test_CreateEllipsisTrimmingSign(void)
{
    DWRITE_BREAK_CONDITION before, after;
    IDWriteTextFormat *format;
    IDWriteInlineObject *sign;
    IDWriteFactory *factory;
    IUnknown *unk;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(format, 1);
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(format, 1);

    hr = IDWriteInlineObject_QueryInterface(sign, &IID_IDWriteTextLayout, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDWriteInlineObject_GetBreakConditions(sign, NULL, NULL);

    before = after = DWRITE_BREAK_CONDITION_CAN_BREAK;
    hr = IDWriteInlineObject_GetBreakConditions(sign, &before, &after);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(before == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", before);
    ok(after == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", after);

    /* Draw tests */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteInlineObject_Draw(sign, NULL, &testrenderer, 0.0, 0.0, FALSE, FALSE, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawellipsis_seq, "ellipsis sign draw test", FALSE);
    IDWriteInlineObject_Release(sign);

    /* non-orthogonal flow/reading combination */
    hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* vista, win7 */, "got 0x%08x\n", hr);
    if (hr == S_OK) {
        hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
        ok(hr == DWRITE_E_FLOWDIRECTIONCONFLICTS, "got 0x%08x\n", hr);
    }

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

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(range.startPosition == 0 && range.length == ~0u, "got %u, %u\n", range.startPosition, range.length);

    range.startPosition = 0;
    range.length = 6;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(range.startPosition == 0 && range.length == 6, "got %u, %u\n", range.startPosition, range.length);

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
    { DRAW_GLYPHRUN, {'s',0}     },
    { DRAW_GLYPHRUN, {'r','i',0} },
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'n',0} },
    { DRAW_GLYPHRUN, {'g',0}     },
    { DRAW_INLINE },
    { DRAW_UNDERLINE },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq2[] = {
    { DRAW_GLYPHRUN, {'s',0} },
    { DRAW_GLYPHRUN, {'t',0} },
    { DRAW_GLYPHRUN, {'r',0} },
    { DRAW_GLYPHRUN, {'i',0} },
    { DRAW_GLYPHRUN, {'n',0} },
    { DRAW_GLYPHRUN, {'g',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq3[] = {
    { DRAW_GLYPHRUN },
    { DRAW_GLYPHRUN, {'a','b',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq4[] = {
    { DRAW_GLYPHRUN, {'s','t','r',0} },
    { DRAW_GLYPHRUN, {'i','n','g',0} },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq5[] = {
    { DRAW_GLYPHRUN, {'s','t',0} },
    { DRAW_GLYPHRUN, {'r','i',0} },
    { DRAW_GLYPHRUN, {'n','g',0} },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry empty_seq[] = {
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_single_run_seq[] = {
    { DRAW_GLYPHRUN, {'s','t','r','i','n','g',0} },
    { DRAW_LAST_KIND }
};

static void test_Draw(void)
{
    static const WCHAR strW[] = {'s','t','r','i','n','g',0};
    static const WCHAR str2W[] = {0x202a,0x202c,'a','b',0};
    static const WCHAR ruW[] = {'r','u',0};
    IDWriteInlineObject *inlineobj;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    DWRITE_MATRIX m;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, ruW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 100.0, 100.0, &layout);
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
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq, "draw test", TRUE);
    IDWriteTextLayout_Release(layout);

    /* with reduced width DrawGlyphRun() is called for every line */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 5.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq2, "draw test 2", TRUE);
    IDWriteTextLayout_Release(layout);

    /* string with control characters */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 4, format, 500.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq3, "draw test 3", TRUE);
    IDWriteTextLayout_Release(layout);

    /* strikethrough splits ranges from renderer point of view, but doesn't break
       shaping */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 500.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);

    range.startPosition = 0;
    range.length = 3;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq4, "draw test 4", FALSE);
    IDWriteTextLayout_Release(layout);

    /* strikethrough somewhere in the middle */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 6, format, 500.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);

    range.startPosition = 2;
    range.length = 2;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq5, "draw test 5", FALSE);
    IDWriteTextLayout_Release(layout);

    /* empty string */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 0, format, 500.0, 100.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, empty_seq, "draw test 6", FALSE);
    IDWriteTextLayout_Release(layout);

    /* different parameter combinations with gdi-compatible layout */
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, TRUE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 7", FALSE);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, NULL, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 8", FALSE);
    IDWriteTextLayout_Release(layout);

    m.m11 = m.m22 = 2.0;
    m.m12 = m.m21 = m.dx = m.dy = 0.0;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, &m, TRUE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 9", FALSE);
    IDWriteTextLayout_Release(layout);

    m.m11 = m.m22 = 2.0;
    m.m12 = m.m21 = m.dx = m.dy = 0.0;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, strW, 6, format, 100.0, 100.0, 1.0, &m, FALSE, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 10", FALSE);
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
    static const WCHAR str3W[] = {0x2066,')',')',0x661,'(',0x627,')',0};
    static const WCHAR str2W[] = {0x202a,0x202c,'a',0};
    static const WCHAR strW[] = {'a','b','c','d',0};
    static const WCHAR str4W[] = {'a',' ',0};
    DWRITE_INLINE_OBJECT_METRICS inline_metrics;
    DWRITE_CLUSTER_METRICS metrics[4];
    IDWriteTextLayout1 *layout1;
    IDWriteInlineObject *trimm;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    UINT32 count, i;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, str3W, 7, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count == 7, "got %u\n", count);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count == 4, "got %u\n", count);

    /* check every cluster width */
    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, sizeof(metrics)/sizeof(metrics[0]), &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 4, "got %u\n", count);
    for (i = 0; i < count; i++) {
        ok(metrics[i].width > 0.0, "%u: got width %.2f\n", i, metrics[i].width);
        ok(metrics[i].length == 1, "%u: got length %u\n", i, metrics[i].length);
    }

    /* apply spacing and check widths again */
    if (IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout1, (void**)&layout1) == S_OK) {
        DWRITE_CLUSTER_METRICS metrics2[4];
        FLOAT leading, trailing, min_advance;
        DWRITE_TEXT_RANGE r;

        leading = trailing = min_advance = 2.0;
        hr = IDWriteTextLayout1_GetCharacterSpacing(layout1, 0, &leading, &trailing,
            &min_advance, NULL);
todo_wine {
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(leading == 0.0 && trailing == 0.0 && min_advance == 0.0,
            "got %.2f, %.2f, %.2f\n", leading, trailing, min_advance);
}
        r.startPosition = 0;
        r.length = 4;
        hr = IDWriteTextLayout1_SetCharacterSpacing(layout1, 10.0, 15.0, 0.0, r);
todo_wine
        ok(hr == S_OK, "got 0x%08x\n", hr);

        count = 0;
        hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics2, sizeof(metrics2)/sizeof(metrics2[0]), &count);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(count == 4, "got %u\n", count);
        for (i = 0; i < count; i++) {
todo_wine
            ok(metrics2[i].width > metrics[i].width, "%u: got width %.2f, was %.2f\n", i, metrics2[i].width,
                metrics[i].width);
            ok(metrics2[i].length == 1, "%u: got length %u\n", i, metrics2[i].length);
        }

        /* back to defaults */
        r.startPosition = 0;
        r.length = 4;
        hr = IDWriteTextLayout1_SetCharacterSpacing(layout1, 0.0, 0.0, 0.0, r);
todo_wine
        ok(hr == S_OK, "got 0x%08x\n", hr);

        IDWriteTextLayout1_Release(layout1);
    }
    else
        win_skip("IDWriteTextLayout1 is not supported, cluster spacing test skipped.\n");

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
    ok(count == 3, "got %u\n", count);

    count = 0;
    memset(&metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count == 3, "got %u\n", count);
    ok(metrics[0].length == 2, "got %u\n", metrics[0].length);

    hr = IDWriteInlineObject_GetMetrics(trimm, &inline_metrics);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine
    ok(inline_metrics.width > 0.0 && inline_metrics.width == metrics[0].width, "got %.2f, expected %.2f\n",
        inline_metrics.width, metrics[0].width);

    IDWriteTextLayout_Release(layout);

    /* text with non-visual control codes */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 3, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* bidi control codes take a separate cluster */
    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 3, "got %u\n", count);

    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 1, "got %d\n", metrics[0].length);
    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    ok(metrics[1].width == 0.0, "got %.2f\n", metrics[1].width);
    ok(metrics[1].length == 1, "got %d\n", metrics[1].length);
    ok(metrics[1].canWrapLineAfter == 0, "got %d\n", metrics[1].canWrapLineAfter);
    ok(metrics[1].isWhitespace == 0, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[1].isNewline == 0, "got %d\n", metrics[1].isNewline);
    ok(metrics[1].isSoftHyphen == 0, "got %d\n", metrics[1].isSoftHyphen);
    ok(metrics[1].isRightToLeft == 0, "got %d\n", metrics[1].isRightToLeft);

    ok(metrics[2].width > 0.0, "got %.2f\n", metrics[2].width);
    ok(metrics[2].length == 1, "got %d\n", metrics[2].length);
todo_wine
    ok(metrics[2].canWrapLineAfter == 1, "got %d\n", metrics[2].canWrapLineAfter);
    ok(metrics[2].isWhitespace == 0, "got %d\n", metrics[2].isWhitespace);
    ok(metrics[2].isNewline == 0, "got %d\n", metrics[2].isNewline);
    ok(metrics[2].isSoftHyphen == 0, "got %d\n", metrics[2].isSoftHyphen);
    ok(metrics[2].isRightToLeft == 0, "got %d\n", metrics[2].isRightToLeft);

    IDWriteTextLayout_Release(layout);

    /* single inline object that fails to report its metrics */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetInlineObject(layout, &testinlineobj, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 1, "got %u\n", count);

    /* object sets a width to 123.0, but returns failure from GetMetrics() */
    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 4, "got %d\n", metrics[0].length);
todo_wine
    ok(metrics[0].canWrapLineAfter == 1, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    /* now set two inline object for [0,1] and [2,3], both fail to report break conditions */
    range.startPosition = 2;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, &testinlineobj2, range);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 2, "got %u\n", count);

    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 2, "got %d\n", metrics[0].length);
    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    ok(metrics[1].width == 0.0, "got %.2f\n", metrics[1].width);
    ok(metrics[1].length == 2, "got %d\n", metrics[1].length);
todo_wine
    ok(metrics[1].canWrapLineAfter == 1, "got %d\n", metrics[1].canWrapLineAfter);
    ok(metrics[1].isWhitespace == 0, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[1].isNewline == 0, "got %d\n", metrics[1].isNewline);
    ok(metrics[1].isSoftHyphen == 0, "got %d\n", metrics[1].isSoftHyphen);
    ok(metrics[1].isRightToLeft == 0, "got %d\n", metrics[1].isRightToLeft);

    IDWriteTextLayout_Release(layout);

    /* zero length string */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 0, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 1;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %u\n", count);
    IDWriteTextLayout_Release(layout);

    /* whitespace */
    hr = IDWriteFactory_CreateTextLayout(factory, str4W, 2, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 2, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 2, "got %u\n", count);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
todo_wine
    ok(metrics[1].isWhitespace == 1, "got %d\n", metrics[1].isWhitespace);
    IDWriteTextLayout_Release(layout);

    IDWriteInlineObject_Release(trimm);
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
    IDWriteFactory_Release(factory);
}

static void test_SetFontSize(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    FLOAT size;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* negative/zero size */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, -15.0, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_SetFontSize(layout, 0.0, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 0, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 10.0, "got %.2f\n", size);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 15.0, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontSize(layout, 123.0, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 1, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size == 15.0, "got %.2f\n", size);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontSize(layout, 15.0, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 1, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size == 15.0, "got %.2f\n", size);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 0, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 15.0, "got %.2f\n", size);

    size = 15.0;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontSize(layout, 20, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 10.0, "got %.2f\n", size);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontSize(layout, 25.0, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    size = 15.0;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontSize(layout, 100, &size, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 25.0, "got %.2f\n", size);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontFamilyName(void)
{
    static const WCHAR taHomaW[] = {'T','a','H','o','m','a',0};
    static const WCHAR arialW[] = {'A','r','i','a','l',0};
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    WCHAR nameW[50];
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* NULL name */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, NULL, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, sizeof(nameW)/sizeof(WCHAR), &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);

    /* set name only different in casing */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, taHomaW, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, arialW, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 0;
    r.length = 0;
    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, sizeof(nameW)/sizeof(WCHAR), &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(nameW, taHomaW), "got %s\n", wine_dbgstr_w(nameW));
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, arialW, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, sizeof(nameW)/sizeof(WCHAR), &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, arialW, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, sizeof(nameW)/sizeof(WCHAR), &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(!lstrcmpW(nameW, arialW), "got name %s\n", wine_dbgstr_w(nameW));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontStyle(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_FONT_STYLE style;
    DWRITE_TEXT_RANGE r;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* invalid style value */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC+1, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 0, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_NORMAL, "got %d\n", style);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_NORMAL, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    style = DWRITE_FONT_STYLE_NORMAL;
    hr = IDWriteTextLayout_GetFontStyle(layout, 1, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(style == DWRITE_FONT_STYLE_ITALIC, "got %d\n", style);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_OBLIQUE, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    style = DWRITE_FONT_STYLE_ITALIC;
    hr = IDWriteTextLayout_GetFontStyle(layout, 1, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    style = DWRITE_FONT_STYLE_ITALIC;
    hr = IDWriteTextLayout_GetFontStyle(layout, 0, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    style = DWRITE_FONT_STYLE_ITALIC;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 20, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_NORMAL, "got %d\n", style);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_OBLIQUE, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    style = DWRITE_FONT_STYLE_NORMAL;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 100, &style, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontStretch(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    DWRITE_FONT_STRETCH stretch;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* invalid stretch value */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_ULTRA_EXPANDED+1, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 0, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_NORMAL, "got %d\n", stretch);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_CONDENSED, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_NORMAL, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 1, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(stretch == DWRITE_FONT_STRETCH_CONDENSED, "got %d\n", stretch);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_EXPANDED, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 1, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 0, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 20, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_NORMAL, "got %d\n", stretch);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_EXPANDED, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 100, &stretch, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    /* trying to set undefined value */
    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_UNDEFINED, r);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetStrikethrough(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    BOOL value;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 0;
    value = TRUE;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 0, &value, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(value == FALSE, "got %d\n", value);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    value = FALSE;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 1, &value, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(value == TRUE, "got %d\n", value);
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    value = TRUE;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 20, &value, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 2 && r.length == ~0u-2, "got %u, %u\n", r.startPosition, r.length);
    ok(value == FALSE, "got %d\n", value);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    value = FALSE;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 100, &value, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(value == TRUE, "got %d\n", value);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_GetMetrics(void)
{
    static const WCHAR str2W[] = {0x2066,')',')',0x661,'(',0x627,')',0};
    static const WCHAR strW[] = {'a','b','c','d',0};
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&metrics, 0xcc, sizeof(metrics));
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width > 0.0, "got %.2f\n", metrics.width);
    ok(metrics.widthIncludingTrailingWhitespace > 0.0, "got %.2f\n", metrics.widthIncludingTrailingWhitespace);
    ok(metrics.height > 0.0, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.maxBidiReorderingDepth == 1, "got %u\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "got %u\n", metrics.lineCount);
}
    IDWriteTextLayout_Release(layout);

    /* a string with more complex bidi sequence */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 7, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&metrics, 0xcc, sizeof(metrics));
    metrics.maxBidiReorderingDepth = 0;
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width > 0.0, "got %.2f\n", metrics.width);
    ok(metrics.widthIncludingTrailingWhitespace > 0.0, "got %.2f\n", metrics.widthIncludingTrailingWhitespace);
    ok(metrics.height > 0.0, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.maxBidiReorderingDepth > 1, "got %u\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "got %u\n", metrics.lineCount);
}
    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFlowDirection(void)
{
    static const WCHAR strW[] = {'a','b','c','d',0};
    DWRITE_READING_DIRECTION reading;
    DWRITE_FLOW_DIRECTION flow;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flow = IDWriteTextFormat_GetFlowDirection(format);
    ok(flow == DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM, "got %d\n", flow);

    reading = IDWriteTextFormat_GetReadingDirection(format);
    ok(reading == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", reading);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* vista,win7 */, "got 0x%08x\n", hr);
    if (hr == S_OK) {
        hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IDWriteTextLayout_Release(layout);

        hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IDWriteTextLayout_Release(layout);
    }
    else
        win_skip("DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT is not supported\n");

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static const struct drawcall_entry draweffect_seq[] = {
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'a','e',0x0300,0} },
    { DRAW_GLYPHRUN, {'d',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect2_seq[] = {
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'a','e',0} },
    { DRAW_GLYPHRUN, {'c','d',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect3_seq[] = {
    { DRAW_INLINE|DRAW_EFFECT },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect4_seq[] = {
    { DRAW_INLINE },
    { DRAW_LAST_KIND }
};

static void test_SetDrawingEffect(void)
{
    static const WCHAR strW[] = {'a','e',0x0300,'d',0}; /* accent grave */
    static const WCHAR str2W[] = {'a','e','c','d',0};
    IDWriteInlineObject *sign;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    IUnknown *unk;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* string with combining mark */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* set effect past the end of text */
    r.startPosition = 100;
    r.length = 10;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetDrawingEffect(layout, 101, &unk, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 100 && r.length == 10, "got %u, %u\n", r.startPosition, r.length);

    r.startPosition = r.length = 0;
    unk = (void*)0xdeadbeef;
    hr = IDWriteTextLayout_GetDrawingEffect(layout, 1000, &unk, &r);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(r.startPosition == 110 && r.length == ~0u-110, "got %u, %u\n", r.startPosition, r.length);
    ok(unk == NULL, "got %p\n", unk);

    /* effect is applied to clusters, not individual text positions */
    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect_seq, "effect draw test", TRUE);
    IDWriteTextLayout_Release(layout);

    /* simple string */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect2_seq, "effect draw test 2", TRUE);
    IDWriteTextLayout_Release(layout);

    /* Inline object - effect set for same range */
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetInlineObject(layout, sign, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect3_seq, "effect draw test 3", FALSE);

    /* now set effect somewhere inside a range replaced by inline object */
    hr = IDWriteTextLayout_SetDrawingEffect(layout, NULL, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* no effect is reported in this case */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect4_seq, "effect draw test 4", FALSE);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, NULL, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    r.startPosition = 0;
    r.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, &testeffect, r);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* first range position is all that matters for inline ranges */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect3_seq, "effect draw test 5", FALSE);

    IDWriteTextLayout_Release(layout);

    IDWriteInlineObject_Release(sign);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static IDWriteFontFace *get_fontface_from_format(IDWriteTextFormat *format)
{
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    IDWriteFontFace *fontface;
    IDWriteFont *font;
    WCHAR nameW[255];
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    hr = IDWriteTextFormat_GetFontCollection(format, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteTextFormat_GetFontFamilyName(format, nameW, sizeof(nameW)/sizeof(WCHAR));
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_FindFamilyName(collection, nameW, &index, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, index, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontCollection_Release(collection);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family,
        IDWriteTextFormat_GetFontWeight(format),
        IDWriteTextFormat_GetFontStretch(format),
        IDWriteTextFormat_GetFontStyle(format),
        &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFont_Release(font);
    IDWriteFontFamily_Release(family);

    return fontface;
}

static void test_GetLineMetrics(void)
{
    static const WCHAR strW[] = {'a','b','c','d',' ',0};
    DWRITE_FONT_METRICS fontmetrics;
    DWRITE_LINE_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, tahomaW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 2048.0, enusW, &format);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 5, format, 30000.0, 1000.0, &layout);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, &metrics, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count == 1, "got count %u\n", count);

    memset(&metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetLineMetrics(layout, &metrics, 1, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine {
    ok(metrics.length == 5, "got %u\n", metrics.length);
    ok(metrics.trailingWhitespaceLength == 1, "got %u\n", metrics.trailingWhitespaceLength);
}
    ok(metrics.newlineLength == 0, "got %u\n", metrics.newlineLength);
    ok(metrics.isTrimmed == FALSE, "got %d\n", metrics.isTrimmed);

    /* Tahoma doesn't provide BASE table, so baseline is calculated from font metrics */
    fontface = get_fontface_from_format(format);
    ok(fontface != NULL, "got %p\n", fontface);
    IDWriteFontFace_GetMetrics(fontface, &fontmetrics);

todo_wine {
    ok(metrics.baseline == fontmetrics.ascent, "got %.2f, expected %d\n", metrics.baseline,
        fontmetrics.ascent);
    ok(metrics.height == fontmetrics.ascent + fontmetrics.descent, "got %.2f, expected %d\n",
        metrics.height, fontmetrics.ascent + fontmetrics.descent);
}
    IDWriteFontFace_Release(fontface);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

START_TEST(layout)
{
    static const WCHAR ctrlstrW[] = {0x202a,0};
    IDWriteFactory *factory;

    if (!(factory = create_factory())) {
        win_skip("failed to create factory\n");
        return;
    }

    /* actual script ids are not fixed */
    get_script_analysis(ctrlstrW, 1, &g_control_sa);

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
    test_SetFontSize();
    test_SetFontFamilyName();
    test_SetFontStyle();
    test_SetFontStretch();
    test_SetStrikethrough();
    test_GetMetrics();
    test_SetFlowDirection();
    test_SetDrawingEffect();
    test_GetLineMetrics();

    IDWriteFactory_Release(factory);
}
