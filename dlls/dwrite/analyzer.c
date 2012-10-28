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
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
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
