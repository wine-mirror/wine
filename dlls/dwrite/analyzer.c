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
    Script_Armenian = 1,
    Script_Balinese = 2,
    Script_Bengali = 3,
    Script_Buginese = 6,
    Script_Canadian = 8,
    Script_Cherokee = 11,
    Script_Controls = 12,
    Script_Coptic = 13,
    Script_Cyrillic = 16,
    Script_Devanagari = 18,
    Script_Ethiopic = 19,
    Script_Georgian = 20,
    Script_Glagolitic = 22,
    Script_Greek = 23,
    Script_Gujarati = 24,
    Script_Gurmukhi = 25,
    Script_Hangul = 27,
    Script_Hebrew = 29,
    Script_Kannada = 32,
    Script_Khmer = 36,
    Script_Lao = 37,
    Script_Latin = 38,
    Script_Lepcha = 39,
    Script_Limbu = 40,
    Script_Malayalam = 44,
    Script_Mongolian = 45,
    Script_Myanmar = 46,
    Script_New_TaiLue = 47,
    Script_NKo = 48,
    Script_Ogham = 49,
    Script_OlChiki = 50,
    Script_Oriya = 53,
    Script_Runic = 58,
    Script_Sinhala = 61,
    Script_Sundanese = 62,
    Script_Syriac = 64,
    Script_TaiLe = 67,
    Script_Tamil = 68,
    Script_Telugu = 69,
    Script_Thaana = 70,
    Script_Thai = 71,
    Script_Tibetan = 72,
    Script_Tifinagh = 73,
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
    { Script_Controls, 0x80, 0x9f },
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
    /* Armenian: U+0530–U+058F */
    { Script_Armenian, 0x530, 0x58f },
    /* Hebrew: U+0590–U+05FF */
    { Script_Hebrew, 0x590, 0x5ff },
    /* Arabic: U+0600–U+06FF */
    { Script_Arabic, 0x600, 0x6ff },
    /* Syriac: U+0600–U+06FF */
    { Script_Syriac, 0x700, 0x74f },
    /* Arabic Supplement: U+0750–U+077F */
    { Script_Arabic, 0x750, 0x77f },
    /* Thaana: U+0780–U+07BF */
    { Script_Thaana, 0x780, 0x7bf },
    /* N'Ko: U+07C0–U+07FF */
    { Script_NKo, 0x7c0, 0x7ff },
    /* Devanagari: U+0900–U+097F */
    { Script_Devanagari, 0x900, 0x97f },
    /* Bengali: U+0980–U+09FF */
    { Script_Bengali, 0x980, 0x9ff },
    /* Gurmukhi: U+0A00–U+0A7F */
    { Script_Gurmukhi, 0xa00, 0xa7f },
    /* Gujarati: U+0A80–U+0AFF */
    { Script_Gujarati, 0xa80, 0xaff },
    /* Oriya: U+0B00–U+0B7F */
    { Script_Oriya, 0xb00, 0xb7f },
    /* Tamil: U+0B80–U+0BFF */
    { Script_Tamil, 0xb80, 0xbff },
    /* Telugu: U+0C00–U+0C7F */
    { Script_Telugu, 0xc00, 0xc7f },
    /* Kannada: U+0C80–U+0CFF */
    { Script_Kannada, 0xc80, 0xcff },
    /* Malayalam: U+0D00–U+0D7F */
    { Script_Malayalam, 0xd00, 0xd7f },
    /* Sinhala: U+0D80–U+0DFF */
    { Script_Sinhala, 0xd80, 0xdff },
    /* Thai: U+0E00–U+0E7F */
    { Script_Thai, 0xe00, 0xe7f },
    /* Lao: U+0E80–U+0EFF */
    { Script_Lao, 0xe80, 0xeff },
    /* Tibetan: U+0F00–U+0FFF */
    { Script_Tibetan, 0xf00, 0xfff },
    /* Myanmar: U+1000–U+109F */
    { Script_Myanmar, 0x1000, 0x109f },
    /* Georgian: U+10A0–U+10FF */
    { Script_Georgian, 0x10a0, 0x10ff },
    /* Hangul Jamo: U+1100–U+11FF */
    { Script_Hangul, 0x1100, 0x11ff },
    /* Ethiopic: U+1200–U+137F */
    /* Ethiopic Extensions: U+1380–U+139F */
    { Script_Ethiopic, 0x1200, 0x139f },
    /* Cherokee: U+13A0–U+13FF */
    { Script_Cherokee, 0x13a0, 0x13ff },
    /* Canadian Aboriginal Syllabics: U+1400–U+167F */
    { Script_Canadian, 0x1400, 0x167f },
    /* Ogham: U+1680–U+169F */
    { Script_Ogham, 0x1680, 0x169f },
    /* Runic: U+16A0–U+16F0 */
    { Script_Runic, 0x16a0, 0x16f0 },
    /* Khmer: U+1780–U+17FF */
    { Script_Khmer, 0x1780, 0x17ff },
    /* Mongolian: U+1800–U+18AF */
    { Script_Mongolian, 0x1800, 0x18af },
    /* Limbu: U+1900–U+194F */
    { Script_Limbu, 0x1900, 0x194f },
    /* Tai Le: U+1950–U+197F */
    { Script_TaiLe, 0x1950, 0x197f },
    /* New Tai Lue: U+1980–U+19DF */
    { Script_New_TaiLue, 0x1980, 0x19df },
    /* Khmer Symbols: U+19E0–U+19FF */
    { Script_Khmer, 0x19e0, 0x19ff },
    /* Buginese: U+1A00–U+1A1F */
    { Script_Buginese, 0x1a00, 0x1a1f },
    /* Tai Tham: U+1A20–U+1AAF */
    { Script_Symbol, 0x1a20, 0x1aaf },
    /* Balinese: U+1B00–U+1B7F */
    { Script_Balinese, 0x1b00, 0x1b7f },
    /* Sundanese: U+1B80–U+1BBF */
    { Script_Sundanese, 0x1b80, 0x1bbf },
    /* Batak: U+1BC0–U+1BFF */
    { Script_Symbol, 0x1bc0, 0x1bff },
    /* Lepcha: U+1C00–U+1C4F */
    { Script_Lepcha, 0x1c00, 0x1c4f },
    /* Ol Chiki: U+1C50–U+1C7F */
    { Script_OlChiki, 0x1c50, 0x1c7f },
    /* Sundanese Supplement: U+1CC0–U+1CCF */
    { Script_Symbol, 0x1cc0, 0x1ccf },
    /* Vedic Extensions: U+1CD0-U+1CFF */
    { Script_Devanagari, 0x1cd0, 0x1cff },
    /* Phonetic Extensions: U+1D00–U+1DBF */
    { Script_Latin, 0x1d00, 0x1dbf },
    /* Combining Diacritical Marks Supplement: U+1DC0–U+1DFF */
    { Script_Symbol, 0x1dc0, 0x1dff },
    /* Latin Extended Additional: U+1E00–U+1EFF */
    { Script_Latin, 0x1e00, 0x1eff },
    /* Greek Extended: U+1F00–U+1F00 */
    { Script_Greek, 0x1f00, 0x1fff },
    /* General Punctuation: U+2000–U+206f */
    /* Superscripts and Subscripts: U+2070–U+209f */
    /* Currency Symbols: U+20A0–U+20CF */
    /* Combining Diacritical Marks for Symbols: U+20D0–U+20FF */
    /* Letterlike Symbols: U+2100–U+214F */
    /* Number Forms: U+2150–U+218F */
    /* Arrows: U+2190–U+21FF */
    /* Mathematical Operators: U+2200–U+22FF */
    /* Miscellaneous Technical: U+2300–U+23FF */
    /* Control Pictures: U+2400–U+243F */
    /* Optical Character Recognition: U+2440–U+245F */
    /* Enclosed Alphanumerics: U+2460–U+24FF */
    /* Box Drawing: U+2500–U+25FF */
    /* Block Elements: U+2580–U+259F */
    /* Geometric Shapes: U+25A0–U+25FF */
    /* Miscellaneous Symbols: U+2600–U+26FF */
    /* Dingbats: U+2700–U+27BF */
    /* Miscellaneous Mathematical Symbols-A: U+27C0–U+27EF */
    /* Supplemental Arrows-A: U+27F0–U+27FF */
    /* Braille Patterns: U+2800–U+28FF */
    /* Supplemental Arrows-B: U+2900–U+297F */
    /* Miscellaneous Mathematical Symbols-B: U+2980–U+29FF */
    /* Supplemental Mathematical Operators: U+2A00–U+2AFF */
    /* Miscellaneous Symbols and Arrows: U+2B00–U+2BFF */
    { Script_Symbol, 0x2000, 0x2bff },
    /* Glagolitic: U+2C00–U+2C5F */
    { Script_Glagolitic, 0x2c00, 0x2c5f },
    /* Latin Extended-C: U+2C60–U+2C7F */
    { Script_Latin, 0x2c60, 0x2c7f },
    /* Coptic: U+2C80–U+2CFF */
    { Script_Coptic, 0x2c80, 0x2cff },
    /* Georgian Supplement: U+2D00–U+2D2F */
    { Script_Georgian, 0x2d00, 0x2d2f },
    /* Tifinagh: U+2D30–U+2D7F */
    { Script_Tifinagh, 0x2d30, 0x2d7f },
    /* unsupported range */
    { Script_Unknown }
};

static UINT16 get_char_script( WCHAR c )
{
    DWORD ch = c;
    unsigned int i;

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
            HRESULT hr;

            sa.shapes = sa.script != Script_Controls ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
            hr = IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
            if (FAILED(hr)) return hr;
            pos = i;
            length = 1;
            sa.script = script;
        }
    }

    /* 1 length case or normal completion call */
    sa.shapes = sa.script != Script_Controls ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
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
