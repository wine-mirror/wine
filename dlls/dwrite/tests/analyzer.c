/*
 *    Text analyzing tests
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

#include <assert.h>

#include "initguid.h"
#include "windows.h"
#include "dwrite.h"

#include "wine/test.h"

static IDWriteFactory *factory;

enum analysis_kind {
    ScriptAnalysis,
    LastKind
};

static const char *get_analysis_kind_name(enum analysis_kind kind)
{
    switch (kind)
    {
    case ScriptAnalysis:
        return "ScriptAnalysis";
    default:
        return "unknown";
    }
}

struct script_analysis {
    UINT32 pos;
    UINT32 len;
    DWRITE_SCRIPT_ANALYSIS a;
};

struct call_entry {
    enum analysis_kind kind;
    struct script_analysis sa;
};

struct testcontext {
    enum analysis_kind kind;
    int todo;
    int *failcount;
    const char *file;
    int line;
};

struct call_sequence
{
    int count;
    int size;
    struct call_entry *sequence;
};

#define NUM_CALL_SEQUENCES    1
#define ANALYZER_ID 0
static struct call_sequence *sequences[NUM_CALL_SEQUENCES];
static struct call_sequence *expected_seq[1];

static void add_call(struct call_sequence **seq, int sequence_index, const struct call_entry *call)
{
    struct call_sequence *call_seq = seq[sequence_index];

    if (!call_seq->sequence)
    {
        call_seq->size = 10;
        call_seq->sequence = HeapAlloc(GetProcessHeap(), 0,
                                      call_seq->size * sizeof (struct call_entry));
    }

    if (call_seq->count == call_seq->size)
    {
        call_seq->size *= 2;
        call_seq->sequence = HeapReAlloc(GetProcessHeap(), 0,
                                        call_seq->sequence,
                                        call_seq->size * sizeof (struct call_entry));
    }

    assert(call_seq->sequence);

    call_seq->sequence[call_seq->count++] = *call;
}

static inline void flush_sequence(struct call_sequence **seg, int sequence_index)
{
    struct call_sequence *call_seq = seg[sequence_index];

    HeapFree(GetProcessHeap(), 0, call_seq->sequence);
    call_seq->sequence = NULL;
    call_seq->count = call_seq->size = 0;
}

static inline void flush_sequences(struct call_sequence **seq, int n)
{
    int i;
    for (i = 0; i < n; i++)
        flush_sequence(seq, i);
}

static void init_call_sequences(struct call_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct call_sequence));
}

static void test_uint(UINT32 actual, UINT32 expected, const char *name, const struct testcontext *ctxt)
{
    if (expected != actual && ctxt->todo)
    {
        (*ctxt->failcount)++;
        ok_(ctxt->file, ctxt->line) (0, "%s: \"%s\" expecting %u, got %u\n", get_analysis_kind_name(ctxt->kind), name, expected, actual);
    }
    else
        ok_(ctxt->file, ctxt->line) (expected == actual, "%s: \"%s\" expecting %u, got %u\n", get_analysis_kind_name(ctxt->kind), name,
            expected, actual);
}

static void ok_sequence_(struct call_sequence **seq, int sequence_index,
    const struct call_entry *expected, const char *context, int todo,
    const char *file, int line)
{
    struct call_sequence *call_seq = seq[sequence_index];
    static const struct call_entry end_of_sequence = { LastKind };
    const struct call_entry *actual, *sequence;
    int failcount = 0;
    struct testcontext ctxt;

    add_call(seq, sequence_index, &end_of_sequence);

    sequence = call_seq->sequence;
    actual = sequence;

    ctxt.failcount = &failcount;
    ctxt.todo = todo;
    ctxt.file = file;
    ctxt.line = line;

    while (expected->kind != LastKind && actual->kind != LastKind)
    {
        if (expected->kind == actual->kind)
        {
            ctxt.kind = expected->kind;

            switch (actual->kind)
            {
            case ScriptAnalysis:
            {
                const struct script_analysis *sa_act = &actual->sa;
                const struct script_analysis *sa_exp = &expected->sa;

                test_uint(sa_act->pos, sa_exp->pos, "position", &ctxt);
                test_uint(sa_act->len, sa_exp->len, "length", &ctxt);
                test_uint(sa_act->a.script, sa_exp->a.script, "script", &ctxt);

                break;
            }
            default:
                ok(0, "%s: callback not handled, %s\n", context, get_analysis_kind_name(actual->kind));
            }
            expected++;
            actual++;
        }
        else if (todo)
        {
            failcount++;
            todo_wine
            {
                ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                    context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            }

            flush_sequence(seq, sequence_index);
            return;
        }
        else
        {
            ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            expected++;
            actual++;
        }
    }

    if (todo)
    {
        todo_wine
        {
            if (expected->kind != LastKind || actual->kind != LastKind)
            {
                failcount++;
                ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
                    context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            }
        }
    }
    else if (expected->kind != LastKind || actual->kind != LastKind)
    {
        ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
            context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
    }

    if (todo && !failcount) /* succeeded yet marked todo */
    {
        todo_wine
        {
            ok_(file, line)(1, "%s: marked \"todo_wine\" but succeeds\n", context);
        }
    }

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)

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
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* scriptanalysis)
{
    struct call_entry entry;

    entry.kind = ScriptAnalysis;
    entry.sa.pos = position;
    entry.sa.len = length;
    entry.sa.a = *scriptanalysis;
    add_call(sequences, ANALYZER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI analysissink_SetLineBreakpoints(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetBidiLevel(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        UINT8 explicitLevel,
        UINT8 resolvedLevel)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetNumberSubstitution(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        IDWriteNumberSubstitution* substitution)
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

struct sa_test {
    const WCHAR string[20];
    int item_count;
    struct script_analysis sa[10];
};

enum scriptcode {
    Script_Arabic = 0,
    Script_C1Controls = 12,
    Script_Coptic = 13,
    Script_Cyrillic = 16,
    Script_Greek = 23,
    Script_Latin  = 38,
    Script_Symbol = 77
};

static struct sa_test sa_tests[] = {
    {
      /* just 1 char string */
      {'t',0}, 1,
          { { 0, 1, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      {'t','e','s','t',0}, 1,
          { { 0, 4, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      {' ',' ',' ',' ','!','$','[','^','{','~',0}, 1,
          { { 0, 10, { Script_Symbol, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      {' ',' ',' ','1','2',' ',0}, 1,
          { { 0, 6, { Script_Symbol, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* digits only */
      {'1','2',0}, 1,
          { { 0, 2, { Script_Symbol, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Arabic */
      {0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,0}, 1,
          { { 0, 7, { Script_Arabic, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Arabic, Latin */
      {'1','2','3','-','5','2',0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,'7','1','.',0}, 1,
          { { 0, 16, { Script_Arabic, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Arabic, English */
      {'A','B','C','-','D','E','F',' ',0x0621,0x0623,0x0624,0}, 2,
          { { 0, 8, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 8, 3, { Script_Arabic, DWRITE_SCRIPT_SHAPES_DEFAULT } },
          }
    },
    {
      /* leading space, Arabic, English */
      {' ',0x0621,0x0623,0x0624,'A','B','C','-','D','E','F',0}, 2,
          { { 0, 4, { Script_Arabic,  DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 4, 7, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } },
          }
    },
    {
      /* English, Arabic, trailing space */
      {'A','B','C','-','D','E','F',0x0621,0x0623,0x0624,' ',0}, 2,
          { { 0, 7, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 7, 4, { Script_Arabic, DWRITE_SCRIPT_SHAPES_DEFAULT } },
          }
    },
    {
      /* C1 Controls, Latin-1 Supplement */
      {0x80,0x90,0x9f,0xa0,0xc0,0xb8,0xbf,0xc0,0xff,0}, 2,
          { { 0, 3, { Script_C1Controls, DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 3, 6, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } },
          }
    },
    {
      /* Latin Extended-A */
      {0x100,0x120,0x130,0x140,0x150,0x160,0x170,0x17f,0}, 1,
          { { 0, 8, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Latin Extended-B */
      {0x180,0x190,0x1bf,0x1c0,0x1c3,0x1c4,0x1cc,0x1dc,0x1ff,0x217,0x21b,0x24f,0}, 1,
          { { 0, 12, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* IPA Extensions */
      {0x250,0x260,0x270,0x290,0x2af,0}, 1,
          { { 0, 5, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Spacing Modifier Letters */
      {0x2b0,0x2ba,0x2d7,0x2dd,0x2ef,0x2ff,0}, 1,
          { { 0, 6, { Script_Latin, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Combining Diacritical Marks */
      {0x300,0x320,0x340,0x345,0x350,0x36f,0}, 1,
          { { 0, 6, { Script_Symbol, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    {
      /* Greek and Coptic */
      {0x370,0x388,0x3d8,0x3e1,0x3e2,0x3fa,0x3ff,0}, 3,
          { { 0, 4, { Script_Greek, DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 4, 1, { Script_Coptic, DWRITE_SCRIPT_SHAPES_DEFAULT } },
            { 5, 2, { Script_Greek, DWRITE_SCRIPT_SHAPES_DEFAULT } }
          }
    },
    {
      /* Cyrillic and Cyrillic Supplement */
      {0x400,0x40f,0x410,0x44f,0x450,0x45f,0x460,0x481,0x48a,0x4f0,0x4fa,0x4ff,0x500,0x510,0x520,0}, 1,
          { { 0, 15, { Script_Cyrillic, DWRITE_SCRIPT_SHAPES_DEFAULT } }}
    },
    /* keep this as end marker */
    { {0} }
};

static void init_expected_sa(struct call_sequence **seq, const struct sa_test *test)
{
    static const struct call_entry end_of_sequence = { LastKind };
    int i;

    flush_sequence(seq, 0);

    /* add expected calls */
    for (i = 0; i < test->item_count; i++)
    {
        struct call_entry call;

        call.kind = ScriptAnalysis;
        call.sa.pos = test->sa[i].pos;
        call.sa.len = test->sa[i].len;
        call.sa.a = test->sa[i].a;
        add_call(seq, 0, &call);
    }

    /* and stop marker */
    add_call(seq, 0, &end_of_sequence);
}

static void test_AnalyzeScript(void)
{
    const struct sa_test *ptr = sa_tests;
    IDWriteTextAnalyzer *analyzer;
    HRESULT hr;

    hr = IDWriteFactory_CreateTextAnalyzer(factory, &analyzer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    while (*ptr->string)
    {
        g_source = ptr->string;

        init_expected_sa(expected_seq, ptr);
        hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &analysissource, 0, lstrlenW(g_source), &analysissink);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok_sequence(sequences, ANALYZER_ID, expected_seq[0]->sequence, "AnalyzeScript", FALSE);
        ptr++;
    }

    IDWriteTextAnalyzer_Release(analyzer);
}

START_TEST(analyzer)
{
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr != S_OK)
    {
        win_skip("failed to create factory\n");
        return;
    }

    init_call_sequences(sequences, NUM_CALL_SEQUENCES);
    init_call_sequences(expected_seq, 1);

    test_AnalyzeScript();

    IDWriteFactory_Release(factory);
}
