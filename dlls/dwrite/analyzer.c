/*
 *    Text analyzer
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

#include "dwrite.h"
#include "dwrite_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

enum scriptcode {
    Script_Arabic = 0,
    Script_C1Controls = 12,
    Script_Coptic = 13,
    Script_Cyrillic = 16,
    Script_Greek = 23,
    Script_Latin  = 38,
    Script_Symbol = 77,
    Script_Unknown = (UINT16)-1
};

struct script_range {
    UINT16 script;
    DWORD first;
    DWORD last;
};

static const struct script_range script_ranges[] = {
    /* C0 Controls: U+0000–U+001F */
    /* ASCII punctuation and symbols: U+0020–U+002F */
    /* ASCII digits: U+0030–U+0039 */
    /* ASCII punctuation and symbols: U+003A–U+0040 */
    { Script_Symbol, 0x00, 0x040 },
    /* Latin uppercase: U+0041–U+005A */
    { Script_Latin, 0x41, 0x5a },
    /* ASCII punctuation and symbols: U+005B–U+0060 */
    { Script_Symbol, 0x5b, 0x060 },
    /* Latin lowercase: U+0061–U+007A */
    { Script_Latin, 0x61, 0x7a },
    /* ASCII punctuation and symbols, control char DEL: U+007B–U+007F */
    { Script_Symbol, 0x7b, 0x7f },
    /* C1 Controls: U+0080–U+009F */
    { Script_C1Controls, 0x80, 0x9f },
    /* Latin-1 Supplement: U+00A0–U+00FF */
    /* Latin Extended-A: U+0100–U+017F */
    /* Latin Extended-B: U+0180–U+024F */
    /* IPA Extensions: U+0250–U+02AF */
    /* Spacing Modifier Letters: U+02B0–U+02FF */
    { Script_Latin, 0xa0, 0x2ff },
    /* Combining Diacritical Marks: U+0300–U+036F */
    { Script_Symbol, 0x300, 0x36f },
    /* Greek: U+0370–U+03E1 */
    { Script_Greek, 0x370, 0x3e1 },
    /* Coptic: U+03E2–U+03Ef */
    { Script_Coptic, 0x3e2, 0x3ef },
    /* Greek: U+03F0–U+03FF */
    { Script_Greek, 0x3f0, 0x3ff },
    /* Cyrillic: U+0400–U+04FF */
    /* Cyrillic Supplement: U+0500–U+052F */
    /* Cyrillic Supplement range is incomplete cause it's based on Unicode 5.2
       that doesn't define some Abkhaz and Azerbaijani letters, we support Unicode 6.0 range here */
    { Script_Cyrillic, 0x400, 0x52f },
    /* Arabic: U+0600–U+06FF */
    { Script_Arabic, 0x600, 0x6ef },
    /* unsupported range */
    { Script_Unknown }
};

static UINT16 get_char_script( WCHAR c )
{
    DWORD ch = c;
    int i;

    for (i = 0; i < sizeof(script_ranges)/sizeof(struct script_range); i++)
    {
        const struct script_range *range = &script_ranges[i];
        if (range->script == Script_Unknown || (range->first <= ch && range->last >= ch))
            return range->script;
    }

    return Script_Unknown;
}

static HRESULT analyze_script(const WCHAR *text, UINT32 len, IDWriteTextAnalysisSink *sink)
{
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT32 pos, i, length;

    if (!len) return S_OK;

    sa.script = get_char_script(*text);
    sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;

    pos = 0;
    length = 1;

    for (i = 1; i < len; i++)
    {
        UINT16 script = get_char_script(text[i]);

        /* Script_Latin_Symb script type is ignored when preceded or followed by another script */
        if (sa.script == Script_Symbol) sa.script = script;
        if (script    == Script_Symbol) script = sa.script;
        /* this is a length of a sequence to be reported next */
        if (sa.script == script) length++;

        if (sa.script != script)
        {
            HRESULT hr = IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
            if (FAILED(hr)) return hr;
            pos = i;
            length = 1;
            sa.script = script;
        }
    }

    /* 1 length case or normal completion call */
    return IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
}

static HRESULT WINAPI dwritetextanalyzer_QueryInterface(IDWriteTextAnalyzer *iface, REFIID riid, void **obj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteTextAnalyzer))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;

}

static ULONG WINAPI dwritetextanalyzer_AddRef(IDWriteTextAnalyzer *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextanalyzer_Release(IDWriteTextAnalyzer *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeScript(IDWriteTextAnalyzer *iface,
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

static HRESULT WINAPI dwritetextanalyzer_AnalyzeBidi(IDWriteTextAnalyzer *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeNumberSubstitution(IDWriteTextAnalyzer *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeLineBreakpoints(IDWriteTextAnalyzer *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphs(IDWriteTextAnalyzer *iface,
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

static HRESULT WINAPI dwritetextanalyzer_GetGlyphPlacements(IDWriteTextAnalyzer *iface,
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

static HRESULT WINAPI dwritetextanalyzer_GetGdiCompatibleGlyphPlacements(IDWriteTextAnalyzer *iface,
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

static const struct IDWriteTextAnalyzerVtbl textanalyzervtbl = {
    dwritetextanalyzer_QueryInterface,
    dwritetextanalyzer_AddRef,
    dwritetextanalyzer_Release,
    dwritetextanalyzer_AnalyzeScript,
    dwritetextanalyzer_AnalyzeBidi,
    dwritetextanalyzer_AnalyzeNumberSubstitution,
    dwritetextanalyzer_AnalyzeLineBreakpoints,
    dwritetextanalyzer_GetGlyphs,
    dwritetextanalyzer_GetGlyphPlacements,
    dwritetextanalyzer_GetGdiCompatibleGlyphPlacements
};

static IDWriteTextAnalyzer textanalyzer = { &textanalyzervtbl };

HRESULT get_textanalyzer(IDWriteTextAnalyzer **ret)
{
    *ret = &textanalyzer;
    return S_OK;
}
