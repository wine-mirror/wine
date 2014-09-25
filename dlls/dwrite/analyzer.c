/*
 *    Text analyzer
 *
 * Copyright 2011 Aric Stewart for CodeWeavers
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
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

#include "dwrite.h"
#include "dwrite_2.h"
#include "dwrite_private.h"
#include "scripts.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

extern const unsigned short wine_linebreak_table[];
extern const unsigned short wine_scripts_table[];

static inline unsigned short get_table_entry(const unsigned short *table, WCHAR ch)
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

static inline UINT16 get_char_script(WCHAR c)
{
    UINT16 script = get_table_entry(wine_scripts_table, c);
    if (script == Script_Unknown) {
        WORD type;
        if (GetStringTypeW(CT_CTYPE1, &c, 1, &type) && (type & C1_CNTRL))
            script = Script_Control;
    }
    return script;
}

static HRESULT analyze_script(const WCHAR *text, UINT32 len, IDWriteTextAnalysisSink *sink)
{
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT32 pos, i, length;

    if (!len) return S_OK;

    sa.script = get_char_script(*text);

    pos = 0;
    length = 1;

    for (i = 1; i < len; i++)
    {
        UINT16 script = get_char_script(text[i]);

        /* Unknown type is ignored when preceded or followed by another script */
        if (sa.script == Script_Unknown) sa.script = script;
        if (script == Script_Unknown && sa.script != Script_Control) script = sa.script;
        /* this is a length of a sequence to be reported next */
        if (sa.script == script) length++;

        if (sa.script != script)
        {
            HRESULT hr;

            sa.shapes = sa.script != Script_Control ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
            hr = IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
            if (FAILED(hr)) return hr;
            pos = i;
            length = 1;
            sa.script = script;
        }
    }

    /* 1 length case or normal completion call */
    sa.shapes = sa.script != Script_Control ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
    return IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
}

struct linebreaking_state {
    DWRITE_LINE_BREAKPOINT *breakpoints;
    UINT32 count;
};

enum BreakConditionLocation {
    BreakConditionBefore,
    BreakConditionAfter
};

enum linebreaking_classes {
    b_BK = 1,
    b_CR,
    b_LF,
    b_CM,
    b_SG,
    b_GL,
    b_CB,
    b_SP,
    b_ZW,
    b_NL,
    b_WJ,
    b_JL,
    b_JV,
    b_JT,
    b_H2,
    b_H3,
    b_XX,
    b_OP,
    b_CL,
    b_CP,
    b_QU,
    b_NS,
    b_EX,
    b_SY,
    b_IS,
    b_PR,
    b_PO,
    b_NU,
    b_AL,
    b_ID,
    b_IN,
    b_HY,
    b_BB,
    b_BA,
    b_SA,
    b_AI,
    b_B2,
    b_HL,
    b_CJ,
    b_RI
};

/* "Can break" is a weak condition, stronger "may not break" and "must break" override it. Initially all conditions are
    set to "can break" and could only be changed once. */
static inline void set_break_condition(UINT32 pos, enum BreakConditionLocation location, DWRITE_BREAK_CONDITION condition,
    struct linebreaking_state *state)
{
    if (location == BreakConditionBefore) {
        if (state->breakpoints[pos].breakConditionBefore != DWRITE_BREAK_CONDITION_CAN_BREAK)
            return;
        state->breakpoints[pos].breakConditionBefore = condition;
        if (pos > 0)
            state->breakpoints[pos-1].breakConditionAfter = condition;
    }
    else {
        if (state->breakpoints[pos].breakConditionAfter != DWRITE_BREAK_CONDITION_CAN_BREAK)
            return;
        state->breakpoints[pos].breakConditionAfter = condition;
        if (pos + 1 < state->count)
            state->breakpoints[pos+1].breakConditionBefore = condition;
    }
}

static HRESULT analyze_linebreaks(const WCHAR *text, UINT32 count, DWRITE_LINE_BREAKPOINT *breakpoints)
{
    struct linebreaking_state state;
    short *break_class;
    int i, j;

    break_class = heap_alloc(count*sizeof(short));
    if (!break_class)
        return E_OUTOFMEMORY;

    state.breakpoints = breakpoints;
    state.count = count;

    /* LB31 - allow breaks everywhere. It will be overridden if needed as
       other rules dictate. */
    for (i = 0; i < count; i++)
    {
        break_class[i] = get_table_entry(wine_linebreak_table, text[i]);

        breakpoints[i].breakConditionBefore = DWRITE_BREAK_CONDITION_CAN_BREAK;
        breakpoints[i].breakConditionAfter  = DWRITE_BREAK_CONDITION_CAN_BREAK;
        breakpoints[i].isWhitespace = break_class[i] == b_BK || break_class[i] == b_ZW || break_class[i] == b_SP || isspaceW(text[i]);
        breakpoints[i].isSoftHyphen = FALSE;
        breakpoints[i].padding = 0;

        /* LB1 - resolve some classes. TODO: use external algorithms for these classes. */
        switch (break_class[i])
        {
            case b_AI:
            case b_SA:
            case b_SG:
            case b_XX:
                break_class[i] = b_AL;
                break;
            case b_CJ:
                break_class[i] = b_NS;
                break;
        }
    }

    /* LB2 - never break at the start */
    set_break_condition(0, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
    /* LB3 - always break at the end. This one is ignored. */

    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB4 - LB6 */
            case b_CR:
                /* LB5 - don't break CR x LF */
                if (i < count-1 && break_class[i+1] == b_LF)
                {
                    set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    break;
                }
            case b_LF:
            case b_NL:
            case b_BK:
                /* LB4 - LB5 - always break after hard breaks */
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MUST_BREAK, &state);
                /* LB6 - do not break before hard breaks */
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB7 - do not break before spaces */
            case b_SP:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_ZW:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB8 - break before character after zero-width space, skip spaces inbetween */
                while (i < count-1 && break_class[i+1] == b_SP)
                    i++;
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
        }
    }

    /* LB9 - LB10 */
    for (i = 0; i < count; i++)
    {
        if (break_class[i] == b_CM)
        {
            if (i > 0)
            {
                switch (break_class[i-1])
                {
                    case b_SP:
                    case b_BK:
                    case b_CR:
                    case b_LF:
                    case b_NL:
                    case b_ZW:
                        break_class[i] = b_AL;
                        break;
                    default:
                        break_class[i] = break_class[i-1];
                }
            }
            else break_class[i] = b_AL;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB11 - don't break before and after word joiner */
            case b_WJ:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB12 - don't break after glue */
            case b_GL:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB12a */
                if (i > 0)
                {
                    if (break_class[i-1] != b_SP && break_class[i-1] != b_BA && break_class[i-1] != b_HY)
                        set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
                break;
            /* LB13 */
            case b_CL:
            case b_CP:
            case b_EX:
            case b_IS:
            case b_SY:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB14 */
            case b_OP:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                while (i < count-1 && break_class[i+1] == b_SP) {
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    i++;
                }
                break;
            /* LB15 */
            case b_QU:
                j = i+1;
                while (j < count-1 && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_OP)
                    for (; j > i; j--)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB16 */
            case b_NS:
                j = i-1;
                while(j > 0 && break_class[j] == b_SP)
                    j--;
                if (break_class[j] == b_CL || break_class[j] == b_CP)
                    for (j++; j <= i; j++)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB17 */
            case b_B2:
                j = i+1;
                while (j < count && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_B2)
                    for (; j > i; j--)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB18 - break is allowed after space */
            case b_SP:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB19 - don't break before or after quotation mark */
            case b_QU:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB20 */
            case b_CB:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB21 */
            case b_BA:
            case b_HY:
            case b_NS:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_BB:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB21a */
            case b_HL:
                if (i < count-2)
                    switch (break_class[i+1])
                    {
                    case b_HY:
                    case b_BA:
                        set_break_condition(i+1, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                break;
            /* LB22 */
            case b_IN:
                if (i > 0)
                {
                    switch (break_class[i-1])
                    {
                        case b_AL:
                        case b_HL:
                        case b_ID:
                        case b_IN:
                        case b_NU:
                            set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                }
                break;
        }

        if (i < count-1)
        {
            /* LB23 */
            if ((break_class[i] == b_ID && break_class[i+1] == b_PO) ||
                (break_class[i] == b_AL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_AL) ||
                (break_class[i] == b_NU && break_class[i+1] == b_HL))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB24 */
            if ((break_class[i] == b_PR && break_class[i+1] == b_ID) ||
                (break_class[i] == b_PR && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PR && break_class[i+1] == b_HL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_HL))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB25 */
            if ((break_class[i] == b_CL && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PR) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PO) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PR) ||
                (break_class[i] == b_PO && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PO && break_class[i+1] == b_NU) ||
                (break_class[i] == b_PR && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PR && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HY && break_class[i+1] == b_NU) ||
                (break_class[i] == b_IS && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_NU) ||
                (break_class[i] == b_SY && break_class[i+1] == b_NU))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB26 */
            if (break_class[i] == b_JL)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }
            if ((break_class[i] == b_JV || break_class[i] == b_H2) &&
                (break_class[i+1] == b_JV || break_class[i+1] == b_JT))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if ((break_class[i] == b_JT || break_class[i] == b_H3) &&
                 break_class[i+1] == b_JT)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB27 */
            switch (break_class[i])
            {
                case b_JL:
                case b_JV:
                case b_JT:
                case b_H2:
                case b_H3:
                    if (break_class[i+1] == b_IN || break_class[i+1] == b_PO)
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            }
            if (break_class[i] == b_PO)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_JT:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }

            /* LB28 */
            if ((break_class[i] == b_AL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_AL && break_class[i+1] == b_HL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB29 */
            if ((break_class[i] == b_IS && break_class[i+1] == b_AL) ||
                (break_class[i] == b_IS && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30 */
            if ((break_class[i] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU) &&
                 break_class[i+1] == b_OP)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if (break_class[i] == b_CP &&
               (break_class[i+1] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30a */
            if (break_class[i] == b_RI && break_class[i+1] == b_RI)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
        }
    }

    heap_free(break_class);
    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer_QueryInterface(IDWriteTextAnalyzer2 *iface, REFIID riid, void **obj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextAnalyzer2) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer1) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;

}

static ULONG WINAPI dwritetextanalyzer_AddRef(IDWriteTextAnalyzer2 *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextanalyzer_Release(IDWriteTextAnalyzer2 *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeScript(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    const WCHAR *text;
    HRESULT hr;
    UINT32 len;

    TRACE("(%p %u %u %p)\n", source, position, length, sink);

    hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &text, &len);
    if (FAILED(hr)) return hr;

    return analyze_script(text, len, sink);
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeBidi(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeNumberSubstitution(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeLineBreakpoints(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    DWRITE_LINE_BREAKPOINT *breakpoints = NULL;
    WCHAR *buff = NULL;
    const WCHAR *text;
    HRESULT hr;
    UINT32 len;

    TRACE("(%p %u %u %p)\n", source, position, length, sink);

    if (length == 0)
        return S_OK;

    /* get some, check for length */
    text = NULL;
    len = 0;
    hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &text, &len);
    if (FAILED(hr)) return hr;

    if (len < length) {
        UINT32 read;

        buff = heap_alloc(length*sizeof(WCHAR));
        if (!buff)
            return E_OUTOFMEMORY;
        memcpy(buff, text, len*sizeof(WCHAR));
        read = len;

        while (read < length && text) {
            text = NULL;
            len = 0;
            hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, read, &text, &len);
            if (FAILED(hr))
                goto done;
            memcpy(&buff[read], text, min(len, length-read)*sizeof(WCHAR));
            read += len;
        }

        text = buff;
    }

    breakpoints = heap_alloc(length*sizeof(*breakpoints));
    if (!breakpoints) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = analyze_linebreaks(text, length, breakpoints);
    if (FAILED(hr))
        goto done;

    hr = IDWriteTextAnalysisSink_SetLineBreakpoints(sink, position, length, breakpoints);

done:
    heap_free(breakpoints);
    heap_free(buff);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphs(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT32 length, IDWriteFontFace* font_face, BOOL is_sideways,
    BOOL is_rtl, DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale,
    IDWriteNumberSubstitution* substitution, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_len, UINT32 feature_ranges, UINT32 max_glyph_count,
    UINT16* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* text_props, UINT16* glyph_indices,
    DWRITE_SHAPING_GLYPH_PROPERTIES* glyph_props, UINT32* actual_glyph_count)
{
    FIXME("(%s:%u %p %d %d %p %s %p %p %p %u %u %p %p %p %p %p): stub\n", debugstr_wn(text, length),
        length, font_face, is_sideways, is_rtl, analysis, debugstr_w(locale), substitution, features, feature_range_len,
        feature_ranges, max_glyph_count, clustermap, text_props, glyph_indices, glyph_props, actual_glyph_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* props,
    UINT32 text_len, UINT16 const* glyph_indices, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace * font_face, FLOAT fontEmSize, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_len, UINT32 feature_ranges, FLOAT* glyph_advances, DWRITE_GLYPH_OFFSET* glyph_offsets)
{
    FIXME("(%s %p %p %u %p %p %u %p %f %d %d %p %s %p %p %u %p %p): stub\n", debugstr_w(text),
        clustermap, props, text_len, glyph_indices, glyph_props, glyph_count, font_face, fontEmSize, is_sideways,
        is_rtl, analysis, debugstr_w(locale), features, feature_range_len, feature_ranges, glyph_advances, glyph_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_GetGdiCompatibleGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* props,
    UINT32 text_len, UINT16 const* glyph_indices, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace * font_face, FLOAT fontEmSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_lengths, UINT32 feature_ranges, FLOAT* glyph_advances, DWRITE_GLYPH_OFFSET* glyph_offsets)
{
    FIXME("(%s %p %p %u %p %p %u %p %f %f %p %d %d %d %p %s %p %p %u %p %p): stub\n", debugstr_w(text),
        clustermap, props, text_len, glyph_indices, glyph_props, glyph_count, font_face, fontEmSize, pixels_per_dip,
        transform, use_gdi_natural, is_sideways, is_rtl, analysis, debugstr_w(locale), features, feature_range_lengths,
        feature_ranges, glyph_advances, glyph_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_ApplyCharacterSpacing(IDWriteTextAnalyzer2 *iface,
    FLOAT leading_spacing, FLOAT trailing_spacing, FLOAT min_advance_width, UINT32 len,
    UINT32 glyph_count, UINT16 const *clustermap, FLOAT const *advances, DWRITE_GLYPH_OFFSET const *offsets,
    DWRITE_SHAPING_GLYPH_PROPERTIES const *props, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    FIXME("(%.2f %.2f %.2f %u %u %p %p %p %p %p %p): stub\n", leading_spacing, trailing_spacing, min_advance_width,
        len, glyph_count, clustermap, advances, offsets, props, modified_advances, modified_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetBaseline(IDWriteTextAnalyzer2 *iface, IDWriteFontFace *face,
    DWRITE_BASELINE baseline, BOOL vertical, BOOL is_simulation_allowed, DWRITE_SCRIPT_ANALYSIS sa,
    const WCHAR *localeName, INT32 *baseline_coord, BOOL *exists)
{
    FIXME("(%p %d %d %u %s %p %p): stub\n", face, vertical, is_simulation_allowed, sa.script, debugstr_w(localeName),
        baseline_coord, exists);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource1* source, UINT32 text_pos, UINT32 len, IDWriteTextAnalysisSink1 *sink)
{
    FIXME("(%p %u %u %p): stub\n", source, text_pos, len, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, DWRITE_MATRIX *transform)
{
    FIXME("(%d %d %p): stub\n", angle, is_sideways, transform);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetScriptProperties(IDWriteTextAnalyzer2 *iface, DWRITE_SCRIPT_ANALYSIS sa,
    DWRITE_SCRIPT_PROPERTIES *props)
{
    FIXME("(%u %p): stub\n", sa.script, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetTextComplexity(IDWriteTextAnalyzer2 *iface, const WCHAR *text,
    UINT32 len, IDWriteFontFace *face, BOOL *is_simple, UINT32 *len_read, UINT16 *indices)
{
    FIXME("(%s:%u %p %p %p %p): stub\n", debugstr_wn(text, len), len, face, is_simple, len_read, indices);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustificationOpportunities(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length, UINT32 glyph_count,
    const WCHAR *text, const UINT16 *clustermap, const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, DWRITE_JUSTIFICATION_OPPORTUNITY *jo)
{
    FIXME("(%p %.2f %u %u %u %s %p %p %p): stub\n", face, font_em_size, sa.script, length, glyph_count,
        debugstr_w(text), clustermap, prop, jo);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_JustifyGlyphAdvances(IDWriteTextAnalyzer2 *iface,
    FLOAT width, UINT32 glyph_count, const DWRITE_JUSTIFICATION_OPPORTUNITY *jo, const FLOAT *advances,
    const DWRITE_GLYPH_OFFSET *offsets, FLOAT *justifiedadvances, DWRITE_GLYPH_OFFSET *justifiedoffsets)
{
    FIXME("(%.2f %u %p %p %p %p %p): stub\n", width, glyph_count, jo, advances, offsets, justifiedadvances,
        justifiedoffsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustifiedGlyphs(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length,
    UINT32 glyph_count, UINT32 max_glyphcount, const UINT16 *clustermap, const UINT16 *indices,
    const FLOAT *advances, const FLOAT *justifiedadvances, const DWRITE_GLYPH_OFFSET *justifiedoffsets,
    const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, UINT32 *actual_count, UINT16 *modified_clustermap,
    UINT16 *modified_indices, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    FIXME("(%p %.2f %u %u %u %u %p %p %p %p %p %p %p %p %p %p %p): stub\n", face, font_em_size, sa.script,
        length, glyph_count, max_glyphcount, clustermap, indices, advances, justifiedadvances, justifiedoffsets,
        prop, actual_count, modified_clustermap, modified_indices, modified_advances, modified_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, FLOAT originX, FLOAT originY, DWRITE_MATRIX *transform)
{
    FIXME("(%d %d %.2f %.2f %p): stub\n", angle, is_sideways, originX, originY, transform);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_GetTypographicFeatures(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *localeName,
    UINT32 max_tagcount, UINT32 *actual_tagcount, DWRITE_FONT_FEATURE_TAG *tags)
{
    FIXME("(%p %u %s %u %p %p): stub\n", face, sa.script, debugstr_w(localeName), max_tagcount, actual_tagcount,
        tags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_CheckTypographicFeature(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *localeName,
    DWRITE_FONT_FEATURE_TAG feature, UINT32 glyph_count, const UINT16 *indices, UINT8 *feature_applies)
{
    FIXME("(%p %u %s %x %u %p %p): stub\n", face, sa.script, debugstr_w(localeName), feature, glyph_count, indices,
        feature_applies);
    return E_NOTIMPL;
}

static const struct IDWriteTextAnalyzer2Vtbl textanalyzervtbl = {
    dwritetextanalyzer_QueryInterface,
    dwritetextanalyzer_AddRef,
    dwritetextanalyzer_Release,
    dwritetextanalyzer_AnalyzeScript,
    dwritetextanalyzer_AnalyzeBidi,
    dwritetextanalyzer_AnalyzeNumberSubstitution,
    dwritetextanalyzer_AnalyzeLineBreakpoints,
    dwritetextanalyzer_GetGlyphs,
    dwritetextanalyzer_GetGlyphPlacements,
    dwritetextanalyzer_GetGdiCompatibleGlyphPlacements,
    dwritetextanalyzer1_ApplyCharacterSpacing,
    dwritetextanalyzer1_GetBaseline,
    dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation,
    dwritetextanalyzer1_GetGlyphOrientationTransform,
    dwritetextanalyzer1_GetScriptProperties,
    dwritetextanalyzer1_GetTextComplexity,
    dwritetextanalyzer1_GetJustificationOpportunities,
    dwritetextanalyzer1_JustifyGlyphAdvances,
    dwritetextanalyzer1_GetJustifiedGlyphs,
    dwritetextanalyzer2_GetGlyphOrientationTransform,
    dwritetextanalyzer2_GetTypographicFeatures,
    dwritetextanalyzer2_CheckTypographicFeature
};

static IDWriteTextAnalyzer2 textanalyzer = { &textanalyzervtbl };

HRESULT get_textanalyzer(IDWriteTextAnalyzer **ret)
{
    *ret = (IDWriteTextAnalyzer*)&textanalyzer;
    return S_OK;
}
