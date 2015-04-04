/*
 *    Text format and layout
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dwrite_private.h"
#include "scripts.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

struct dwrite_textformat_data {
    WCHAR *family_name;
    UINT32 family_len;
    WCHAR *locale;
    UINT32 locale_len;

    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;

    DWRITE_PARAGRAPH_ALIGNMENT paralign;
    DWRITE_READING_DIRECTION readingdir;
    DWRITE_WORD_WRAPPING wrapping;
    DWRITE_TEXT_ALIGNMENT textalignment;
    DWRITE_FLOW_DIRECTION flow;
    DWRITE_LINE_SPACING_METHOD spacingmethod;
    DWRITE_VERTICAL_GLYPH_ORIENTATION vertical_orientation;

    FLOAT spacing;
    FLOAT baseline;
    FLOAT fontsize;

    DWRITE_TRIMMING trimming;
    IDWriteInlineObject *trimmingsign;

    IDWriteFontCollection *collection;
    IDWriteFontFallback *fallback;
};

enum layout_range_attr_kind {
    LAYOUT_RANGE_ATTR_WEIGHT,
    LAYOUT_RANGE_ATTR_STYLE,
    LAYOUT_RANGE_ATTR_STRETCH,
    LAYOUT_RANGE_ATTR_FONTSIZE,
    LAYOUT_RANGE_ATTR_EFFECT,
    LAYOUT_RANGE_ATTR_INLINE,
    LAYOUT_RANGE_ATTR_UNDERLINE,
    LAYOUT_RANGE_ATTR_STRIKETHROUGH,
    LAYOUT_RANGE_ATTR_PAIR_KERNING,
    LAYOUT_RANGE_ATTR_FONTCOLL,
    LAYOUT_RANGE_ATTR_LOCALE,
    LAYOUT_RANGE_ATTR_FONTFAMILY
};

struct layout_range_attr_value {
    DWRITE_TEXT_RANGE range;
    union {
        DWRITE_FONT_WEIGHT weight;
        DWRITE_FONT_STYLE style;
        DWRITE_FONT_STRETCH stretch;
        FLOAT fontsize;
        IDWriteInlineObject *object;
        IUnknown *effect;
        BOOL underline;
        BOOL strikethrough;
        BOOL pair_kerning;
        IDWriteFontCollection *collection;
        const WCHAR *locale;
        const WCHAR *fontfamily;
    } u;
};

struct layout_range {
    struct list entry;
    DWRITE_TEXT_RANGE range;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    FLOAT fontsize;
    DWRITE_FONT_STRETCH stretch;
    IDWriteInlineObject *object;
    IUnknown *effect;
    BOOL underline;
    BOOL strikethrough;
    BOOL pair_kerning;
    IDWriteFontCollection *collection;
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    WCHAR *fontfamily;
};

enum layout_run_kind {
    LAYOUT_RUN_REGULAR,
    LAYOUT_RUN_INLINE
};

struct inline_object_run {
    IDWriteInlineObject *object;
    UINT16 length;
};

struct regular_layout_run {
    DWRITE_GLYPH_RUN_DESCRIPTION descr;
    DWRITE_GLYPH_RUN run;
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT16 *glyphs;
    UINT16 *clustermap;
    FLOAT  *advances;
    DWRITE_GLYPH_OFFSET *offsets;
};

struct layout_run {
    struct list entry;
    enum layout_run_kind kind;
    union {
        struct inline_object_run object;
        struct regular_layout_run regular;
    } u;
};

enum layout_recompute_mask {
    RECOMPUTE_NOMINAL_RUNS  = 1 << 0,
    RECOMPUTE_MINIMAL_WIDTH = 1 << 1,
    RECOMPUTE_EVERYTHING    = 0xffff
};

struct dwrite_textlayout {
    IDWriteTextLayout2 IDWriteTextLayout2_iface;
    IDWriteTextFormat1 IDWriteTextFormat1_iface;
    IDWriteTextAnalysisSink IDWriteTextAnalysisSink_iface;
    IDWriteTextAnalysisSource IDWriteTextAnalysisSource_iface;
    LONG ref;

    WCHAR *str;
    UINT32 len;
    struct dwrite_textformat_data format;
    FLOAT  maxwidth;
    FLOAT  maxheight;
    struct list ranges;
    struct list runs;
    USHORT recompute;

    DWRITE_LINE_BREAKPOINT *nominal_breakpoints;
    DWRITE_LINE_BREAKPOINT *actual_breakpoints;

    DWRITE_CLUSTER_METRICS *clusters;
    UINT32 clusters_count;
    FLOAT  minwidth;

    /* gdi-compatible layout specifics */
    BOOL   gdicompatible;
    FLOAT  pixels_per_dip;
    BOOL   use_gdi_natural;
    DWRITE_MATRIX transform;
};

struct dwrite_textformat {
    IDWriteTextFormat1 IDWriteTextFormat1_iface;
    LONG ref;
    struct dwrite_textformat_data format;
};

struct dwrite_trimmingsign {
    IDWriteInlineObject IDWriteInlineObject_iface;
    LONG ref;
};

struct dwrite_typography {
    IDWriteTypography IDWriteTypography_iface;
    LONG ref;

    DWRITE_FONT_FEATURE *features;
    UINT32 allocated;
    UINT32 count;
};

static const IDWriteTextFormat1Vtbl dwritetextformatvtbl;

static void release_format_data(struct dwrite_textformat_data *data)
{
    if (data->collection) IDWriteFontCollection_Release(data->collection);
    if (data->fallback) IDWriteFontFallback_Release(data->fallback);
    if (data->trimmingsign) IDWriteInlineObject_Release(data->trimmingsign);
    heap_free(data->family_name);
    heap_free(data->locale);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextLayout2(IDWriteTextLayout2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextLayout2_iface);
}

static inline struct dwrite_textlayout *impl_layout_form_IDWriteTextFormat1(IDWriteTextFormat1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextFormat1_iface);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextAnalysisSink(IDWriteTextAnalysisSink *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextAnalysisSink_iface);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextAnalysisSource(IDWriteTextAnalysisSource *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextAnalysisSource_iface);
}

static inline struct dwrite_textformat *impl_from_IDWriteTextFormat1(IDWriteTextFormat1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textformat, IDWriteTextFormat1_iface);
}

static inline struct dwrite_trimmingsign *impl_from_IDWriteInlineObject(IDWriteInlineObject *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_trimmingsign, IDWriteInlineObject_iface);
}

static inline struct dwrite_typography *impl_from_IDWriteTypography(IDWriteTypography *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_typography, IDWriteTypography_iface);
}

static inline const char *debugstr_run(const struct regular_layout_run *run)
{
    return wine_dbg_sprintf("[%u,%u]", run->descr.textPosition, run->descr.textPosition +
        run->descr.stringLength);
}

static HRESULT get_fontfallback_from_format(const struct dwrite_textformat_data *format, IDWriteFontFallback **fallback)
{
    *fallback = format->fallback;
    if (*fallback)
        IDWriteFontFallback_AddRef(*fallback);
    return S_OK;
}

static HRESULT set_fontfallback_for_format(struct dwrite_textformat_data *format, IDWriteFontFallback *fallback)
{
    if (format->fallback)
        IDWriteFontFallback_Release(format->fallback);
    format->fallback = fallback;
    if (fallback)
        IDWriteFontFallback_AddRef(fallback);
    return S_OK;
}

static struct layout_run *alloc_layout_run(enum layout_run_kind kind)
{
    struct layout_run *ret;

    ret = heap_alloc(sizeof(*ret));
    if (!ret) return NULL;

    memset(ret, 0, sizeof(*ret));
    ret->kind = kind;
    if (kind == LAYOUT_RUN_REGULAR) {
        ret->u.regular.sa.script = Script_Unknown;
        ret->u.regular.sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    }

    return ret;
}

static void free_layout_runs(struct dwrite_textlayout *layout)
{
    struct layout_run *cur, *cur2;
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->runs, struct layout_run, entry) {
        list_remove(&cur->entry);
        if (cur->kind == LAYOUT_RUN_REGULAR) {
            if (cur->u.regular.run.fontFace)
                IDWriteFontFace_Release(cur->u.regular.run.fontFace);
            heap_free(cur->u.regular.glyphs);
            heap_free(cur->u.regular.clustermap);
            heap_free(cur->u.regular.advances);
            heap_free(cur->u.regular.offsets);
        }
        heap_free(cur);
    }
}

/* Used to resolve break condition by forcing stronger condition over weaker. */
static inline DWRITE_BREAK_CONDITION override_break_condition(DWRITE_BREAK_CONDITION existingbreak, DWRITE_BREAK_CONDITION newbreak)
{
    switch (existingbreak) {
    case DWRITE_BREAK_CONDITION_NEUTRAL:
        return newbreak;
    case DWRITE_BREAK_CONDITION_CAN_BREAK:
        return newbreak == DWRITE_BREAK_CONDITION_NEUTRAL ? existingbreak : newbreak;
    /* let's keep stronger conditions as is */
    case DWRITE_BREAK_CONDITION_MAY_NOT_BREAK:
    case DWRITE_BREAK_CONDITION_MUST_BREAK:
        break;
    default:
        ERR("unknown break condition %d\n", existingbreak);
    }

    return existingbreak;
}

/* Actual breakpoint data gets updated with break condition required by inline object set for range 'cur'. */
static HRESULT layout_update_breakpoints_range(struct dwrite_textlayout *layout, const struct layout_range *cur)
{
    DWRITE_BREAK_CONDITION before, after;
    HRESULT hr;
    UINT32 i;

    hr = IDWriteInlineObject_GetBreakConditions(cur->object, &before, &after);
    if (FAILED(hr))
        return hr;

    if (!layout->actual_breakpoints) {
        layout->actual_breakpoints = heap_alloc(sizeof(DWRITE_LINE_BREAKPOINT)*layout->len);
        if (!layout->actual_breakpoints)
            return E_OUTOFMEMORY;
    }
    memcpy(layout->actual_breakpoints, layout->nominal_breakpoints, sizeof(DWRITE_LINE_BREAKPOINT)*layout->len);

    for (i = cur->range.startPosition; i < cur->range.length + cur->range.startPosition; i++) {
        UINT32 j = i + cur->range.startPosition;
        if (i == 0) {
            if (j)
                layout->actual_breakpoints[j].breakConditionBefore = layout->actual_breakpoints[j-1].breakConditionAfter =
                    override_break_condition(layout->actual_breakpoints[j-1].breakConditionAfter, before);
            else
                layout->actual_breakpoints[j].breakConditionBefore = before;

            layout->actual_breakpoints[j].breakConditionAfter = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
        }

        layout->actual_breakpoints[j].isWhitespace = 0;
        layout->actual_breakpoints[j].isSoftHyphen = 0;

        if (i == cur->range.length - 1) {
            layout->actual_breakpoints[j].breakConditionBefore = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
            if (j < layout->len - 1)
                layout->actual_breakpoints[j].breakConditionAfter = layout->actual_breakpoints[j+1].breakConditionAfter =
                    override_break_condition(layout->actual_breakpoints[j+1].breakConditionAfter, before);
            else
                layout->actual_breakpoints[j].breakConditionAfter = before;
        }
    }

    return S_OK;
}

static struct layout_range *get_layout_range_by_pos(struct dwrite_textlayout *layout, UINT32 pos);

static inline DWRITE_LINE_BREAKPOINT get_effective_breakpoint(const struct dwrite_textlayout *layout, UINT32 pos)
{
    if (layout->actual_breakpoints)
        return layout->actual_breakpoints[pos];
    return layout->nominal_breakpoints[pos];
}

static inline void init_cluster_metrics(const struct dwrite_textlayout *layout, const struct regular_layout_run *run,
    UINT16 start_glyph, UINT16 stop_glyph, UINT32 stop_position, DWRITE_CLUSTER_METRICS *metrics)
{
    UINT8 breakcondition;
    UINT16 j;

    metrics->width = 0.0;
    for (j = start_glyph; j < stop_glyph; j++)
        metrics->width += run->run.glyphAdvances[j];
    metrics->length = 0;

    if (stop_glyph == run->run.glyphCount)
        breakcondition = get_effective_breakpoint(layout, stop_position).breakConditionAfter;
    else
        breakcondition = get_effective_breakpoint(layout, stop_position).breakConditionBefore;

    metrics->canWrapLineAfter = breakcondition == DWRITE_BREAK_CONDITION_CAN_BREAK ||
                                breakcondition == DWRITE_BREAK_CONDITION_MUST_BREAK;
    metrics->isWhitespace = FALSE; /* FIXME */
    metrics->isNewline = FALSE;    /* FIXME */
    metrics->isSoftHyphen = FALSE; /* FIXME */
    metrics->isRightToLeft = run->run.bidiLevel & 1;
    metrics->padding = 0;
}

/*

  All clusters in a 'run' will be added to 'layout' data, starting at index pointed to by 'cluster'.
  On return 'cluster' is updated to point to next metrics struct to be filled in on next call.
  Note that there's no need to reallocate anything at this point as we allocate one cluster per
  codepoint initially.

*/
static void layout_set_cluster_metrics(struct dwrite_textlayout *layout, const struct regular_layout_run *run, UINT32 *cluster)
{
    DWRITE_CLUSTER_METRICS *metrics = &layout->clusters[*cluster];
    UINT32 i, start = 0;

    for (i = 0; i < run->descr.stringLength; i++) {
        BOOL end = i == run->descr.stringLength - 1;

        if (run->descr.clusterMap[start] != run->descr.clusterMap[i]) {
            init_cluster_metrics(layout, run, run->descr.clusterMap[start], run->descr.clusterMap[i], i, metrics);
            metrics->length = i - start;

            *cluster += 1;
            metrics++;
            start = i;
        }

        if (end) {
            init_cluster_metrics(layout, run, run->descr.clusterMap[start], run->run.glyphCount, i, metrics);
            metrics->length = i - start + 1;

            *cluster += 1;
            return;
        }
    }
}

static HRESULT layout_compute_runs(struct dwrite_textlayout *layout)
{
    IDWriteTextAnalyzer *analyzer;
    struct layout_range *range;
    struct layout_run *r;
    UINT32 cluster = 0;
    HRESULT hr;

    free_layout_runs(layout);
    heap_free(layout->clusters);
    layout->clusters_count = 0;
    layout->clusters = heap_alloc(layout->len*sizeof(DWRITE_CLUSTER_METRICS));
    if (!layout->clusters)
        return E_OUTOFMEMORY;

    hr = get_textanalyzer(&analyzer);
    if (FAILED(hr))
        return hr;

    LIST_FOR_EACH_ENTRY(range, &layout->ranges, struct layout_range, entry) {
        /* inline objects override actual text in a range */
        if (range->object) {
            hr = layout_update_breakpoints_range(layout, range);
            if (FAILED(hr))
                return hr;

            r = alloc_layout_run(LAYOUT_RUN_INLINE);
            if (!r)
                return E_OUTOFMEMORY;

            r->u.object.object = range->object;
            r->u.object.length = range->range.length;
            list_add_tail(&layout->runs, &r->entry);
            continue;
        }

        /* initial splitting by script */
        hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &layout->IDWriteTextAnalysisSource_iface,
            range->range.startPosition, range->range.length, &layout->IDWriteTextAnalysisSink_iface);
        if (FAILED(hr))
            break;

        /* this splits it further */
        hr = IDWriteTextAnalyzer_AnalyzeBidi(analyzer, &layout->IDWriteTextAnalysisSource_iface,
            range->range.startPosition, range->range.length, &layout->IDWriteTextAnalysisSink_iface);
        if (FAILED(hr))
            break;
    }

    /* fill run info */
    LIST_FOR_EACH_ENTRY(r, &layout->runs, struct layout_run, entry) {
        DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props = NULL;
        DWRITE_SHAPING_TEXT_PROPERTIES *text_props = NULL;
        struct regular_layout_run *run = &r->u.regular;
        IDWriteFontFamily *family;
        UINT32 index, max_count;
        IDWriteFont *font;
        BOOL exists = TRUE;

        /* we need to do very little in case of inline objects */
        if (r->kind == LAYOUT_RUN_INLINE) {
            DWRITE_CLUSTER_METRICS *metrics = &layout->clusters[cluster];
            DWRITE_INLINE_OBJECT_METRICS inlinemetrics;

            metrics->width = 0.0;
            metrics->length = r->u.object.length;
            metrics->canWrapLineAfter = FALSE;
            metrics->isWhitespace = FALSE;
            metrics->isNewline = FALSE;
            metrics->isSoftHyphen = FALSE;
            metrics->isRightToLeft = FALSE;
            metrics->padding = 0;
            cluster++;

            /* TODO: is it fatal if GetMetrics() fails? */
            hr = IDWriteInlineObject_GetMetrics(r->u.object.object, &inlinemetrics);
            if (FAILED(hr)) {
                FIXME("failed to get inline object metrics, 0x%08x\n", hr);
                continue;
            }
            metrics->width = inlinemetrics.width;

            /* FIXME: use resolved breakpoints in this case too */

            continue;
        }

        range = get_layout_range_by_pos(layout, run->descr.textPosition);

        hr = IDWriteFontCollection_FindFamilyName(range->collection, range->fontfamily, &index, &exists);
        if (FAILED(hr) || !exists) {
            WARN("%s: family %s not found in collection %p\n", debugstr_run(run), debugstr_w(range->fontfamily), range->collection);
            continue;
        }

        hr = IDWriteFontCollection_GetFontFamily(range->collection, index, &family);
        if (FAILED(hr))
            continue;

        hr = IDWriteFontFamily_GetFirstMatchingFont(family, range->weight, range->stretch, range->style, &font);
        IDWriteFontFamily_Release(family);
        if (FAILED(hr)) {
            WARN("%s: failed to get a matching font\n", debugstr_run(run));
            continue;
        }

        hr = IDWriteFont_CreateFontFace(font, &run->run.fontFace);
        IDWriteFont_Release(font);
        if (FAILED(hr))
            continue;

        run->run.fontEmSize = range->fontsize;
        run->descr.localeName = range->locale;
        run->clustermap = heap_alloc(run->descr.stringLength*sizeof(UINT16));

        max_count = 3*run->descr.stringLength/2 + 16;
        run->glyphs = heap_alloc(max_count*sizeof(UINT16));
        if (!run->clustermap || !run->glyphs)
            goto memerr;

        text_props = heap_alloc(run->descr.stringLength*sizeof(DWRITE_SHAPING_TEXT_PROPERTIES));
        glyph_props = heap_alloc(max_count*sizeof(DWRITE_SHAPING_GLYPH_PROPERTIES));
        if (!text_props || !glyph_props)
            goto memerr;

        while (1) {
            hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, run->descr.string, run->descr.stringLength,
                run->run.fontFace, run->run.isSideways, run->run.bidiLevel & 1, &run->sa, run->descr.localeName,
                NULL /* FIXME */, NULL, NULL, 0, max_count, run->clustermap, text_props, run->glyphs, glyph_props,
                &run->run.glyphCount);
            if (hr == E_NOT_SUFFICIENT_BUFFER) {
                heap_free(run->glyphs);
                heap_free(glyph_props);

                max_count = run->run.glyphCount;

                run->glyphs = heap_alloc(max_count*sizeof(UINT16));
                glyph_props = heap_alloc(max_count*sizeof(DWRITE_SHAPING_GLYPH_PROPERTIES));
                if (!run->glyphs || !glyph_props)
                    goto memerr;

                continue;
            }

            break;
        }

        if (FAILED(hr)) {
            heap_free(text_props);
            heap_free(glyph_props);
            WARN("%s: shaping failed 0x%08x\n", debugstr_run(run), hr);
            continue;
        }

        run->run.glyphIndices = run->glyphs;
        run->descr.clusterMap = run->clustermap;

        run->advances = heap_alloc(run->run.glyphCount*sizeof(FLOAT));
        run->offsets = heap_alloc(run->run.glyphCount*sizeof(DWRITE_GLYPH_OFFSET));
        if (!run->advances || !run->offsets)
            goto memerr;

        /* now set advances and offsets */
        if (layout->gdicompatible)
            hr = IDWriteTextAnalyzer_GetGdiCompatibleGlyphPlacements(analyzer, run->descr.string, run->descr.clusterMap,
                text_props, run->descr.stringLength, run->run.glyphIndices, glyph_props, run->run.glyphCount,
                run->run.fontFace, run->run.fontEmSize, layout->pixels_per_dip, &layout->transform, layout->use_gdi_natural,
                run->run.isSideways, run->run.bidiLevel & 1, &run->sa, run->descr.localeName, NULL, NULL, 0,
                run->advances, run->offsets);
        else
            hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, run->descr.string, run->descr.clusterMap, text_props,
                run->descr.stringLength, run->run.glyphIndices, glyph_props, run->run.glyphCount, run->run.fontFace,
                run->run.fontEmSize, run->run.isSideways, run->run.bidiLevel & 1, &run->sa, run->descr.localeName,
                NULL, NULL, 0, run->advances, run->offsets);

        heap_free(text_props);
        heap_free(glyph_props);
        if (FAILED(hr))
            WARN("%s: failed to get glyph placement info, 0x%08x\n", debugstr_run(run), hr);

        run->run.glyphAdvances = run->advances;
        run->run.glyphOffsets = run->offsets;

        layout_set_cluster_metrics(layout, run, &cluster);

        continue;

    memerr:
        heap_free(text_props);
        heap_free(glyph_props);
        heap_free(run->clustermap);
        heap_free(run->glyphs);
        heap_free(run->advances);
        heap_free(run->offsets);
        run->advances = NULL;
        run->offsets = NULL;
        run->clustermap = run->glyphs = NULL;
        hr = E_OUTOFMEMORY;
        break;
    }

    if (hr == S_OK)
        layout->clusters_count = cluster;

    IDWriteTextAnalyzer_Release(analyzer);
    return hr;
}

static HRESULT layout_compute(struct dwrite_textlayout *layout)
{
    HRESULT hr;

    if (!(layout->recompute & RECOMPUTE_NOMINAL_RUNS))
        return S_OK;

    /* nominal breakpoints are evaluated only once, because string never changes */
    if (!layout->nominal_breakpoints) {
        IDWriteTextAnalyzer *analyzer;
        HRESULT hr;

        layout->nominal_breakpoints = heap_alloc(sizeof(DWRITE_LINE_BREAKPOINT)*layout->len);
        if (!layout->nominal_breakpoints)
            return E_OUTOFMEMORY;

        hr = get_textanalyzer(&analyzer);
        if (FAILED(hr))
            return hr;

        hr = IDWriteTextAnalyzer_AnalyzeLineBreakpoints(analyzer, &layout->IDWriteTextAnalysisSource_iface,
            0, layout->len, &layout->IDWriteTextAnalysisSink_iface);
        IDWriteTextAnalyzer_Release(analyzer);
    }
    if (layout->actual_breakpoints) {
        heap_free(layout->actual_breakpoints);
        layout->actual_breakpoints = NULL;
    }

    hr = layout_compute_runs(layout);

    if (TRACE_ON(dwrite)) {
        struct layout_run *cur;

        LIST_FOR_EACH_ENTRY(cur, &layout->runs, struct layout_run, entry) {
            if (cur->kind == LAYOUT_RUN_INLINE)
                TRACE("run inline object %p, len %u\n", cur->u.object.object, cur->u.object.length);
            else
                TRACE("run [%u,%u], len %u, bidilevel %u\n", cur->u.regular.descr.textPosition, cur->u.regular.descr.textPosition +
                    cur->u.regular.descr.stringLength-1, cur->u.regular.descr.stringLength, cur->u.regular.run.bidiLevel);
        }
    }

    layout->recompute &= ~RECOMPUTE_NOMINAL_RUNS;
    return hr;
}

/* To be used in IDWriteTextLayout methods to validate and fix passed range */
static inline BOOL validate_text_range(struct dwrite_textlayout *layout, DWRITE_TEXT_RANGE *r)
{
    if (r->startPosition >= layout->len)
        return FALSE;

    if (r->startPosition + r->length > layout->len)
        r->length = layout->len - r->startPosition;

    return TRUE;
}

static BOOL is_same_layout_attrvalue(struct layout_range const *range, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        return range->weight == value->u.weight;
    case LAYOUT_RANGE_ATTR_STYLE:
        return range->style == value->u.style;
    case LAYOUT_RANGE_ATTR_STRETCH:
        return range->stretch == value->u.stretch;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        return range->fontsize == value->u.fontsize;
    case LAYOUT_RANGE_ATTR_INLINE:
        return range->object == value->u.object;
    case LAYOUT_RANGE_ATTR_EFFECT:
        return range->effect == value->u.effect;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        return range->underline == value->u.underline;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        return range->strikethrough == value->u.strikethrough;
    case LAYOUT_RANGE_ATTR_PAIR_KERNING:
        return range->pair_kerning == value->u.pair_kerning;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        return range->collection == value->u.collection;
    case LAYOUT_RANGE_ATTR_LOCALE:
        return strcmpW(range->locale, value->u.locale) == 0;
    case LAYOUT_RANGE_ATTR_FONTFAMILY:
        return strcmpW(range->fontfamily, value->u.fontfamily) == 0;
    default:
        ;
    }

    return FALSE;
}

static inline BOOL is_same_layout_attributes(struct layout_range const *left, struct layout_range const *right)
{
    return left->weight == right->weight &&
           left->style  == right->style &&
           left->stretch == right->stretch &&
           left->fontsize == right->fontsize &&
           left->object == right->object &&
           left->effect == right->effect &&
           left->underline == right->underline &&
           left->strikethrough == right->strikethrough &&
           left->pair_kerning == right->pair_kerning &&
           left->collection == right->collection &&
          !strcmpW(left->locale, right->locale) &&
          !strcmpW(left->fontfamily, right->fontfamily);
}

static inline BOOL is_same_text_range(const DWRITE_TEXT_RANGE *left, const DWRITE_TEXT_RANGE *right)
{
    return left->startPosition == right->startPosition && left->length == right->length;
}

/* Allocates range and inits it with default values from text format. */
static struct layout_range *alloc_layout_range(struct dwrite_textlayout *layout, const DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;

    range = heap_alloc(sizeof(*range));
    if (!range) return NULL;

    range->range = *r;
    range->weight = layout->format.weight;
    range->style  = layout->format.style;
    range->stretch = layout->format.stretch;
    range->fontsize = layout->format.fontsize;
    range->object = NULL;
    range->effect = NULL;
    range->underline = FALSE;
    range->strikethrough = FALSE;
    range->pair_kerning = FALSE;

    range->fontfamily = heap_strdupW(layout->format.family_name);
    if (!range->fontfamily) {
        heap_free(range);
        return NULL;
    }

    range->collection = layout->format.collection;
    if (range->collection)
        IDWriteFontCollection_AddRef(range->collection);
    strcpyW(range->locale, layout->format.locale);

    return range;
}

static struct layout_range *alloc_layout_range_from(struct layout_range *from, const DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;

    range = heap_alloc(sizeof(*range));
    if (!range) return NULL;

    *range = *from;
    range->range = *r;

    range->fontfamily = heap_strdupW(from->fontfamily);
    if (!range->fontfamily) {
        heap_free(range);
        return NULL;
    }

    /* update refcounts */
    if (range->object)
        IDWriteInlineObject_AddRef(range->object);
    if (range->effect)
        IUnknown_AddRef(range->effect);
    if (range->collection)
        IDWriteFontCollection_AddRef(range->collection);

    return range;
}

static void free_layout_range(struct layout_range *range)
{
    if (!range)
        return;
    if (range->object)
        IDWriteInlineObject_Release(range->object);
    if (range->effect)
        IUnknown_Release(range->effect);
    if (range->collection)
        IDWriteFontCollection_Release(range->collection);
    heap_free(range->fontfamily);
    heap_free(range);
}

static void free_layout_ranges_list(struct dwrite_textlayout *layout)
{
    struct layout_range *cur, *cur2;
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->ranges, struct layout_range, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }
}

static struct layout_range *find_outer_range(struct dwrite_textlayout *layout, const DWRITE_TEXT_RANGE *range)
{
    struct layout_range *cur;

    LIST_FOR_EACH_ENTRY(cur, &layout->ranges, struct layout_range, entry) {

        if (cur->range.startPosition > range->startPosition)
            return NULL;

        if ((cur->range.startPosition + cur->range.length < range->startPosition + range->length) &&
            (range->startPosition < cur->range.startPosition + cur->range.length))
            return NULL;
        if (cur->range.startPosition + cur->range.length >= range->startPosition + range->length)
            return cur;
    }

    return NULL;
}

static struct layout_range *get_layout_range_by_pos(struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range *cur;

    LIST_FOR_EACH_ENTRY(cur, &layout->ranges, struct layout_range, entry) {
        DWRITE_TEXT_RANGE *r = &cur->range;
        if (r->startPosition <= pos && pos < r->startPosition + r->length)
            return cur;
    }

    return NULL;
}

static inline BOOL set_layout_range_iface_attr(IUnknown **dest, IUnknown *value)
{
    if (*dest == value) return FALSE;

    if (*dest)
        IUnknown_Release(*dest);
    *dest = value;
    if (*dest)
        IUnknown_AddRef(*dest);

    return TRUE;
}

static BOOL set_layout_range_attrval(struct layout_range *dest, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    BOOL changed = FALSE;

    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        changed = dest->weight != value->u.weight;
        dest->weight = value->u.weight;
        break;
    case LAYOUT_RANGE_ATTR_STYLE:
        changed = dest->style != value->u.style;
        dest->style = value->u.style;
        break;
    case LAYOUT_RANGE_ATTR_STRETCH:
        changed = dest->stretch != value->u.stretch;
        dest->stretch = value->u.stretch;
        break;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        changed = dest->fontsize != value->u.fontsize;
        dest->fontsize = value->u.fontsize;
        break;
    case LAYOUT_RANGE_ATTR_INLINE:
        changed = set_layout_range_iface_attr((IUnknown**)&dest->object, (IUnknown*)value->u.object);
        break;
    case LAYOUT_RANGE_ATTR_EFFECT:
        changed = set_layout_range_iface_attr((IUnknown**)&dest->effect, (IUnknown*)value->u.effect);
        break;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        changed = dest->underline != value->u.underline;
        dest->underline = value->u.underline;
        break;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        changed = dest->strikethrough != value->u.strikethrough;
        dest->strikethrough = value->u.strikethrough;
        break;
    case LAYOUT_RANGE_ATTR_PAIR_KERNING:
        changed = dest->pair_kerning != value->u.pair_kerning;
        dest->pair_kerning = value->u.pair_kerning;
        break;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        changed = set_layout_range_iface_attr((IUnknown**)&dest->collection, (IUnknown*)value->u.collection);
        break;
    case LAYOUT_RANGE_ATTR_LOCALE:
        changed = strcmpW(dest->locale, value->u.locale) != 0;
        if (changed)
            strcpyW(dest->locale, value->u.locale);
        break;
    case LAYOUT_RANGE_ATTR_FONTFAMILY:
        changed = strcmpW(dest->fontfamily, value->u.fontfamily) != 0;
        if (changed) {
            heap_free(dest->fontfamily);
            dest->fontfamily = heap_strdupW(value->u.fontfamily);
        }
        break;
    default:
        ;
    }

    return changed;
}

static inline BOOL is_in_layout_range(const DWRITE_TEXT_RANGE *outer, const DWRITE_TEXT_RANGE *inner)
{
    return (inner->startPosition >= outer->startPosition) &&
           (inner->startPosition + inner->length <= outer->startPosition + outer->length);
}

static inline HRESULT return_range(const struct layout_range *range, DWRITE_TEXT_RANGE *r)
{
    if (r) *r = range->range;
    return S_OK;
}

/* Set attribute value for given range, does all needed splitting/merging of existing ranges. */
static HRESULT set_layout_range_attr(struct dwrite_textlayout *layout, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    struct layout_range *outer, *right, *left, *cur;
    struct list *ranges = &layout->ranges;
    BOOL changed = FALSE;
    DWRITE_TEXT_RANGE r;

    if (!validate_text_range(layout, &value->range))
        return S_OK;

    /* If new range is completely within existing range, split existing range in two */
    if ((outer = find_outer_range(layout, &value->range))) {

        /* no need to add same range */
        if (is_same_layout_attrvalue(outer, attr, value))
            return S_OK;

        /* for matching range bounds just replace data */
        if (is_same_text_range(&outer->range, &value->range)) {
            changed = set_layout_range_attrval(outer, attr, value);
            goto done;
        }

        /* add new range to the left */
        if (value->range.startPosition == outer->range.startPosition) {
            left = alloc_layout_range_from(outer, &value->range);
            if (!left) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(left, attr, value);
            list_add_before(&outer->entry, &left->entry);
            outer->range.startPosition += value->range.length;
            outer->range.length -= value->range.length;
            goto done;
        }

        /* add new range to the right */
        if (value->range.startPosition + value->range.length == outer->range.startPosition + outer->range.length) {
            right = alloc_layout_range_from(outer, &value->range);
            if (!right) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(right, attr, value);
            list_add_after(&outer->entry, &right->entry);
            outer->range.length -= value->range.length;
            goto done;
        }

        r.startPosition = value->range.startPosition + value->range.length;
        r.length = outer->range.length + outer->range.startPosition - r.startPosition;

        /* right part */
        right = alloc_layout_range_from(outer, &r);
        /* new range in the middle */
        cur = alloc_layout_range_from(outer, &value->range);
        if (!right || !cur) {
            free_layout_range(right);
            free_layout_range(cur);
            return E_OUTOFMEMORY;
        }

        /* reuse container range as a left part */
        outer->range.length = value->range.startPosition - outer->range.startPosition;

        /* new part */
        set_layout_range_attrval(cur, attr, value);

        list_add_after(&outer->entry, &cur->entry);
        list_add_after(&cur->entry, &right->entry);

        return S_OK;
    }

    /* Now it's only possible that given range contains some existing ranges, fully or partially.
       Update all of them. */
    left = get_layout_range_by_pos(layout, value->range.startPosition);
    if (left->range.startPosition == value->range.startPosition)
        changed = set_layout_range_attrval(left, attr, value);
    else /* need to split */ {
        r.startPosition = value->range.startPosition;
        r.length = left->range.length - value->range.startPosition + left->range.startPosition;
        left->range.length -= r.length;
        cur = alloc_layout_range_from(left, &r);
        changed = set_layout_range_attrval(cur, attr, value);
        list_add_after(&left->entry, &cur->entry);
    }
    cur = LIST_ENTRY(list_next(ranges, &left->entry), struct layout_range, entry);

    /* for all existing ranges covered by new one update value */
    while (is_in_layout_range(&value->range, &cur->range)) {
        changed = set_layout_range_attrval(cur, attr, value);
        cur = LIST_ENTRY(list_next(ranges, &cur->entry), struct layout_range, entry);
    }

    /* it's possible rightmost range intersects */
    if (cur && (cur->range.startPosition < value->range.startPosition + value->range.length)) {
        r.startPosition = cur->range.startPosition;
        r.length = value->range.startPosition + value->range.length - cur->range.startPosition;
        left = alloc_layout_range_from(cur, &r);
        changed = set_layout_range_attrval(left, attr, value);
        cur->range.startPosition += left->range.length;
        cur->range.length -= left->range.length;
        list_add_before(&cur->entry, &left->entry);
    }

done:
    if (changed) {
        struct list *next, *i;

        layout->recompute = RECOMPUTE_EVERYTHING;
        i = list_head(ranges);
        while ((next = list_next(ranges, i))) {
            struct layout_range *next_range = LIST_ENTRY(next, struct layout_range, entry);

            cur = LIST_ENTRY(i, struct layout_range, entry);
            if (is_same_layout_attributes(cur, next_range)) {
                /* remove similar range */
                cur->range.length += next_range->range.length;
                list_remove(next);
                free_layout_range(next_range);
            }
            else
                i = list_next(ranges, i);
        }
    }

    return S_OK;
}

static inline const WCHAR *get_string_attribute_ptr(struct layout_range *range, enum layout_range_attr_kind kind)
{
    const WCHAR *str;

    switch (kind) {
        case LAYOUT_RANGE_ATTR_LOCALE:
            str = range->locale;
            break;
        case LAYOUT_RANGE_ATTR_FONTFAMILY:
            str = range->fontfamily;
            break;
        default:
            str = NULL;
    }

    return str;
}

static HRESULT get_string_attribute_length(struct dwrite_textlayout *layout, enum layout_range_attr_kind kind, UINT32 position,
    UINT32 *length, DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;
    const WCHAR *str;

    range = get_layout_range_by_pos(layout, position);
    if (!range) {
        *length = 0;
        return S_OK;
    }

    str = get_string_attribute_ptr(range, kind);
    *length = strlenW(str);
    return return_range(range, r);
}

static HRESULT get_string_attribute_value(struct dwrite_textlayout *layout, enum layout_range_attr_kind kind, UINT32 position,
    WCHAR *ret, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;
    const WCHAR *str;

    if (length == 0)
        return E_INVALIDARG;

    ret[0] = 0;
    range = get_layout_range_by_pos(layout, position);
    if (!range)
        return E_INVALIDARG;

    str = get_string_attribute_ptr(range, kind);
    if (length < strlenW(str) + 1)
        return E_NOT_SUFFICIENT_BUFFER;

    strcpyW(ret, str);
    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_QueryInterface(IDWriteTextLayout2 *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IDWriteTextLayout2) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout1) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDWriteTextFormat1) ||
             IsEqualIID(riid, &IID_IDWriteTextFormat))
        *obj = &This->IDWriteTextFormat1_iface;

    if (*obj) {
        IDWriteTextLayout2_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_AddRef(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextlayout_Release(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        free_layout_ranges_list(This);
        free_layout_runs(This);
        release_format_data(&This->format);
        heap_free(This->nominal_breakpoints);
        heap_free(This->actual_breakpoints);
        heap_free(This->clusters);
        heap_free(This->str);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextlayout_SetTextAlignment(IDWriteTextLayout2 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    return IDWriteTextFormat1_SetTextAlignment(&This->IDWriteTextFormat1_iface, alignment);
}

static HRESULT WINAPI dwritetextlayout_SetParagraphAlignment(IDWriteTextLayout2 *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    return IDWriteTextFormat1_SetParagraphAlignment(&This->IDWriteTextFormat1_iface, alignment);
}

static HRESULT WINAPI dwritetextlayout_SetWordWrapping(IDWriteTextLayout2 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d)\n", This, wrapping);
    return IDWriteTextFormat1_SetWordWrapping(&This->IDWriteTextFormat1_iface, wrapping);
}

static HRESULT WINAPI dwritetextlayout_SetReadingDirection(IDWriteTextLayout2 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    return IDWriteTextFormat1_SetReadingDirection(&This->IDWriteTextFormat1_iface, direction);
}

static HRESULT WINAPI dwritetextlayout_SetFlowDirection(IDWriteTextLayout2 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    return IDWriteTextFormat1_SetFlowDirection(&This->IDWriteTextFormat1_iface, direction);
}

static HRESULT WINAPI dwritetextlayout_SetIncrementalTabStop(IDWriteTextLayout2 *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%.2f)\n", This, tabstop);
    return IDWriteTextFormat1_SetIncrementalTabStop(&This->IDWriteTextFormat1_iface, tabstop);
}

static HRESULT WINAPI dwritetextlayout_SetTrimming(IDWriteTextLayout2 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p %p)\n", This, trimming, trimming_sign);
    return IDWriteTextFormat1_SetTrimming(&This->IDWriteTextFormat1_iface, trimming, trimming_sign);
}

static HRESULT WINAPI dwritetextlayout_SetLineSpacing(IDWriteTextLayout2 *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d %.2f %.2f)\n", This, spacing, line_spacing, baseline);
    return IDWriteTextFormat1_SetLineSpacing(&This->IDWriteTextFormat1_iface, spacing, line_spacing, baseline);
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextlayout_GetTextAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetTextAlignment(&This->IDWriteTextFormat1_iface);
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextlayout_GetParagraphAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetParagraphAlignment(&This->IDWriteTextFormat1_iface);
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextlayout_GetWordWrapping(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetWordWrapping(&This->IDWriteTextFormat1_iface);
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_GetReadingDirection(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetReadingDirection(&This->IDWriteTextFormat1_iface);
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextlayout_GetFlowDirection(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFlowDirection(&This->IDWriteTextFormat1_iface);
}

static FLOAT WINAPI dwritetextlayout_GetIncrementalTabStop(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetIncrementalTabStop(&This->IDWriteTextFormat1_iface);
}

static HRESULT WINAPI dwritetextlayout_GetTrimming(IDWriteTextLayout2 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p %p)\n", This, options, trimming_sign);
    return IDWriteTextFormat1_GetTrimming(&This->IDWriteTextFormat1_iface, options, trimming_sign);
}

static HRESULT WINAPI dwritetextlayout_GetLineSpacing(IDWriteTextLayout2 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);
    return IDWriteTextFormat1_GetLineSpacing(&This->IDWriteTextFormat1_iface, method, spacing, baseline);
}

static HRESULT WINAPI dwritetextlayout_GetFontCollection(IDWriteTextLayout2 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p)\n", This, collection);
    return IDWriteTextFormat1_GetFontCollection(&This->IDWriteTextFormat1_iface, collection);
}

static UINT32 WINAPI dwritetextlayout_GetFontFamilyNameLength(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFontFamilyNameLength(&This->IDWriteTextFormat1_iface);
}

static HRESULT WINAPI dwritetextlayout_GetFontFamilyName(IDWriteTextLayout2 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p %u)\n", This, name, size);
    return IDWriteTextFormat1_GetFontFamilyName(&This->IDWriteTextFormat1_iface, name, size);
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextlayout_GetFontWeight(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFontWeight(&This->IDWriteTextFormat1_iface);
}

static DWRITE_FONT_STYLE WINAPI dwritetextlayout_GetFontStyle(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFontStyle(&This->IDWriteTextFormat1_iface);
}

static DWRITE_FONT_STRETCH WINAPI dwritetextlayout_GetFontStretch(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFontStretch(&This->IDWriteTextFormat1_iface);
}

static FLOAT WINAPI dwritetextlayout_GetFontSize(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetFontSize(&This->IDWriteTextFormat1_iface);
}

static UINT32 WINAPI dwritetextlayout_GetLocaleNameLength(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return IDWriteTextFormat1_GetLocaleNameLength(&This->IDWriteTextFormat1_iface);
}

static HRESULT WINAPI dwritetextlayout_GetLocaleName(IDWriteTextLayout2 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p %u)\n", This, name, size);
    return IDWriteTextFormat1_GetLocaleName(&This->IDWriteTextFormat1_iface, name, size);
}

static HRESULT WINAPI dwritetextlayout_SetMaxWidth(IDWriteTextLayout2 *iface, FLOAT maxWidth)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%.1f)\n", This, maxWidth);

    if (maxWidth < 0.0)
        return E_INVALIDARG;

    This->maxwidth = maxWidth;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetMaxHeight(IDWriteTextLayout2 *iface, FLOAT maxHeight)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%.1f)\n", This, maxHeight);

    if (maxHeight < 0.0)
        return E_INVALIDARG;

    This->maxheight = maxHeight;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetFontCollection(IDWriteTextLayout2 *iface, IDWriteFontCollection* collection, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%p %s)\n", This, collection, debugstr_range(&range));

    value.range = range;
    value.u.collection = collection;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_FONTCOLL, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontFamilyName(IDWriteTextLayout2 *iface, WCHAR const *name, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(name), debugstr_range(&range));

    if (!name)
        return E_INVALIDARG;

    value.range = range;
    value.u.fontfamily = name;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_FONTFAMILY, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontWeight(IDWriteTextLayout2 *iface, DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, weight, debugstr_range(&range));

    value.range = range;
    value.u.weight = weight;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_WEIGHT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStyle(IDWriteTextLayout2 *iface, DWRITE_FONT_STYLE style, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, style, debugstr_range(&range));

    value.range = range;
    value.u.style = style;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STYLE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStretch(IDWriteTextLayout2 *iface, DWRITE_FONT_STRETCH stretch, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, stretch, debugstr_range(&range));

    value.range = range;
    value.u.stretch = stretch;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STRETCH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontSize(IDWriteTextLayout2 *iface, FLOAT size, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%.2f %s)\n", This, size, debugstr_range(&range));

    value.range = range;
    value.u.fontsize = size;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_FONTSIZE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetUnderline(IDWriteTextLayout2 *iface, BOOL underline, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, underline, debugstr_range(&range));

    value.range = range;
    value.u.underline = underline;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_UNDERLINE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetStrikethrough(IDWriteTextLayout2 *iface, BOOL strikethrough, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, strikethrough, debugstr_range(&range));

    value.range = range;
    value.u.underline = strikethrough;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STRIKETHROUGH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetDrawingEffect(IDWriteTextLayout2 *iface, IUnknown* effect, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%p %s)\n", This, effect, debugstr_range(&range));

    value.range = range;
    value.u.effect = effect;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_EFFECT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetInlineObject(IDWriteTextLayout2 *iface, IDWriteInlineObject *object, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%p %s)\n", This, object, debugstr_range(&range));

    value.range = range;
    value.u.object = object;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_INLINE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetTypography(IDWriteTextLayout2 *iface, IDWriteTypography* typography, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %s): stub\n", This, typography, debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetLocaleName(IDWriteTextLayout2 *iface, WCHAR const* locale, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(locale), debugstr_range(&range));

    if (!locale || strlenW(locale) > LOCALE_NAME_MAX_LENGTH-1)
        return E_INVALIDARG;

    value.range = range;
    value.u.locale = locale;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_LOCALE, &value);
}

static FLOAT WINAPI dwritetextlayout_GetMaxWidth(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->maxwidth;
}

static FLOAT WINAPI dwritetextlayout_GetMaxHeight(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->maxheight;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontCollection(IDWriteTextLayout2 *iface, UINT32 position,
    IDWriteFontCollection** collection, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, collection, r);

    range = get_layout_range_by_pos(This, position);
    *collection = range ? range->collection : NULL;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyNameLength(IDWriteTextLayout2 *iface,
    UINT32 position, UINT32 *length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%d %p %p)\n", This, position, length, r);
    return get_string_attribute_length(This, LAYOUT_RANGE_ATTR_FONTFAMILY, position, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyName(IDWriteTextLayout2 *iface,
    UINT32 position, WCHAR *name, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%u %p %u %p)\n", This, position, name, length, r);
    return get_string_attribute_value(This, LAYOUT_RANGE_ATTR_FONTFAMILY, position, name, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontWeight(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_WEIGHT *weight, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, weight, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *weight = range->weight;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStyle(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_STYLE *style, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, style, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *style = range->style;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStretch(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_STRETCH *stretch, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, stretch, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *stretch = range->stretch;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontSize(IDWriteTextLayout2 *iface,
    UINT32 position, FLOAT *size, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, size, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *size = range->fontsize;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetUnderline(IDWriteTextLayout2 *iface,
    UINT32 position, BOOL *underline, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, underline, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *underline = range->underline;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetStrikethrough(IDWriteTextLayout2 *iface,
    UINT32 position, BOOL *strikethrough, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, strikethrough, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *strikethrough = range->strikethrough;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetDrawingEffect(IDWriteTextLayout2 *iface,
    UINT32 position, IUnknown **effect, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, effect, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *effect = range->effect;
    if (*effect)
        IUnknown_AddRef(*effect);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetInlineObject(IDWriteTextLayout2 *iface,
    UINT32 position, IDWriteInlineObject **object, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, object, r);

    range = get_layout_range_by_pos(This, position);
    *object = range ? range->object : NULL;
    if (*object)
        IDWriteInlineObject_AddRef(*object);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetTypography(IDWriteTextLayout2 *iface,
    UINT32 position, IDWriteTypography** typography, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, typography, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleNameLength(IDWriteTextLayout2 *iface,
    UINT32 position, UINT32* length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%u %p %p)\n", This, position, length, r);
    return get_string_attribute_length(This, LAYOUT_RANGE_ATTR_LOCALE, position, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleName(IDWriteTextLayout2 *iface,
    UINT32 position, WCHAR* locale, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%u %p %u %p)\n", This, position, locale, length, r);
    return get_string_attribute_value(This, LAYOUT_RANGE_ATTR_LOCALE, position, locale, length, r);
}

static HRESULT WINAPI dwritetextlayout_Draw(IDWriteTextLayout2 *iface,
    void *context, IDWriteTextRenderer* renderer, FLOAT originX, FLOAT originY)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %p %f %f): stub\n", This, context, renderer, originX, originY);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetLineMetrics(IDWriteTextLayout2 *iface,
    DWRITE_LINE_METRICS *metrics, UINT32 max_count, UINT32 *actual_count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %u %p): stub\n", This, metrics, max_count, actual_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetMetrics(IDWriteTextLayout2 *iface, DWRITE_TEXT_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    DWRITE_TEXT_METRICS1 metrics1;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, metrics);

    hr = IDWriteTextLayout2_GetMetrics(iface, &metrics1);
    if (hr == S_OK)
        memcpy(metrics, &metrics1, sizeof(*metrics));

    return hr;
}

static HRESULT WINAPI dwritetextlayout_GetOverhangMetrics(IDWriteTextLayout2 *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, overhangs);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetClusterMetrics(IDWriteTextLayout2 *iface,
    DWRITE_CLUSTER_METRICS *metrics, UINT32 max_count, UINT32 *count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    HRESULT hr;

    TRACE("(%p)->(%p %u %p)\n", This, metrics, max_count, count);

    hr = layout_compute(This);
    if (FAILED(hr))
        return hr;

    if (metrics)
        memcpy(metrics, This->clusters, sizeof(DWRITE_CLUSTER_METRICS)*min(max_count, This->clusters_count));

    *count = This->clusters_count;
    return max_count >= This->clusters_count ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

/* Only to be used with DetermineMinWidth() to find the longest cluster sequence that we don't want to try
   too hard to break. */
static inline BOOL is_terminal_cluster(struct dwrite_textlayout *layout, UINT32 index)
{
    if (layout->clusters[index].isWhitespace || layout->clusters[index].isNewline ||
       (index == layout->clusters_count - 1))
        return TRUE;
    /* check next one */
    return (index < layout->clusters_count - 1) && layout->clusters[index+1].isWhitespace;
}

static HRESULT WINAPI dwritetextlayout_DetermineMinWidth(IDWriteTextLayout2 *iface, FLOAT* min_width)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FLOAT width;
    HRESULT hr;
    UINT32 i;

    TRACE("(%p)->(%p)\n", This, min_width);

    if (!min_width)
        return E_INVALIDARG;

    if (!(This->recompute & RECOMPUTE_MINIMAL_WIDTH))
        goto width_done;

    *min_width = 0.0;
    hr = layout_compute(This);
    if (FAILED(hr))
        return hr;

    for (i = 0; i < This->clusters_count;) {
        if (is_terminal_cluster(This, i)) {
            width = This->clusters[i].width;
            i++;
        }
        else {
            width = 0.0;
            while (!is_terminal_cluster(This, i)) {
                width += This->clusters[i].width;
                i++;
            }
            /* count last one too */
            width += This->clusters[i].width;
        }

        if (width > This->minwidth)
            This->minwidth = width;
    }
    This->recompute &= ~RECOMPUTE_MINIMAL_WIDTH;

width_done:
    *min_width = This->minwidth;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_HitTestPoint(IDWriteTextLayout2 *iface,
    FLOAT pointX, FLOAT pointY, BOOL* is_trailinghit, BOOL* is_inside, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%f %f %p %p %p): stub\n", This, pointX, pointY, is_trailinghit, is_inside, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextPosition(IDWriteTextLayout2 *iface,
    UINT32 textPosition, BOOL is_trailinghit, FLOAT* pointX, FLOAT* pointY, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %d %p %p %p): stub\n", This, textPosition, is_trailinghit, pointX, pointY, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextRange(IDWriteTextLayout2 *iface,
    UINT32 textPosition, UINT32 textLength, FLOAT originX, FLOAT originY,
    DWRITE_HIT_TEST_METRICS *metrics, UINT32 max_metricscount, UINT32* actual_metricscount)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %u %f %f %p %u %p): stub\n", This, textPosition, textLength, originX, originY, metrics,
        max_metricscount, actual_metricscount);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_SetPairKerning(IDWriteTextLayout2 *iface, BOOL is_pairkerning_enabled,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, is_pairkerning_enabled, debugstr_range(&range));

    value.range = range;
    value.u.pair_kerning = !!is_pairkerning_enabled;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_PAIR_KERNING, &value);
}

static HRESULT WINAPI dwritetextlayout1_GetPairKerning(IDWriteTextLayout2 *iface, UINT32 position, BOOL *is_pairkerning_enabled,
        DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, is_pairkerning_enabled, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *is_pairkerning_enabled = range->pair_kerning;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout1_SetCharacterSpacing(IDWriteTextLayout2 *iface, FLOAT leading_spacing, FLOAT trailing_spacing,
    FLOAT minimum_advance_width, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%f %f %f %s): stub\n", This, leading_spacing, trailing_spacing, minimum_advance_width, debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_GetCharacterSpacing(IDWriteTextLayout2 *iface, UINT32 position, FLOAT* leading_spacing,
    FLOAT* trailing_spacing, FLOAT* minimum_advance_width, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %p %p %p): stub\n", This, position, leading_spacing, trailing_spacing, minimum_advance_width, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout2_GetMetrics(IDWriteTextLayout2 *iface, DWRITE_TEXT_METRICS1 *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout2_SetVerticalGlyphOrientation(IDWriteTextLayout2 *iface, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%d)\n", This, orientation);

    if ((UINT32)orientation > DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED)
        return E_INVALIDARG;

    This->format.vertical_orientation = orientation;
    return S_OK;
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextlayout2_GetVerticalGlyphOrientation(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.vertical_orientation;
}

static HRESULT WINAPI dwritetextlayout2_SetLastLineWrapping(IDWriteTextLayout2 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, lastline_wrapping_enabled);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritetextlayout2_GetLastLineWrapping(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritetextlayout2_SetOpticalAlignment(IDWriteTextLayout2 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextlayout2_GetOpticalAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_OPTICAL_ALIGNMENT_NONE;
}

static HRESULT WINAPI dwritetextlayout2_SetFontFallback(IDWriteTextLayout2 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return set_fontfallback_for_format(&This->format, fallback);
}

static HRESULT WINAPI dwritetextlayout2_GetFontFallback(IDWriteTextLayout2 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return get_fontfallback_from_format(&This->format, fallback);
}

static const IDWriteTextLayout2Vtbl dwritetextlayoutvtbl = {
    dwritetextlayout_QueryInterface,
    dwritetextlayout_AddRef,
    dwritetextlayout_Release,
    dwritetextlayout_SetTextAlignment,
    dwritetextlayout_SetParagraphAlignment,
    dwritetextlayout_SetWordWrapping,
    dwritetextlayout_SetReadingDirection,
    dwritetextlayout_SetFlowDirection,
    dwritetextlayout_SetIncrementalTabStop,
    dwritetextlayout_SetTrimming,
    dwritetextlayout_SetLineSpacing,
    dwritetextlayout_GetTextAlignment,
    dwritetextlayout_GetParagraphAlignment,
    dwritetextlayout_GetWordWrapping,
    dwritetextlayout_GetReadingDirection,
    dwritetextlayout_GetFlowDirection,
    dwritetextlayout_GetIncrementalTabStop,
    dwritetextlayout_GetTrimming,
    dwritetextlayout_GetLineSpacing,
    dwritetextlayout_GetFontCollection,
    dwritetextlayout_GetFontFamilyNameLength,
    dwritetextlayout_GetFontFamilyName,
    dwritetextlayout_GetFontWeight,
    dwritetextlayout_GetFontStyle,
    dwritetextlayout_GetFontStretch,
    dwritetextlayout_GetFontSize,
    dwritetextlayout_GetLocaleNameLength,
    dwritetextlayout_GetLocaleName,
    dwritetextlayout_SetMaxWidth,
    dwritetextlayout_SetMaxHeight,
    dwritetextlayout_SetFontCollection,
    dwritetextlayout_SetFontFamilyName,
    dwritetextlayout_SetFontWeight,
    dwritetextlayout_SetFontStyle,
    dwritetextlayout_SetFontStretch,
    dwritetextlayout_SetFontSize,
    dwritetextlayout_SetUnderline,
    dwritetextlayout_SetStrikethrough,
    dwritetextlayout_SetDrawingEffect,
    dwritetextlayout_SetInlineObject,
    dwritetextlayout_SetTypography,
    dwritetextlayout_SetLocaleName,
    dwritetextlayout_GetMaxWidth,
    dwritetextlayout_GetMaxHeight,
    dwritetextlayout_layout_GetFontCollection,
    dwritetextlayout_layout_GetFontFamilyNameLength,
    dwritetextlayout_layout_GetFontFamilyName,
    dwritetextlayout_layout_GetFontWeight,
    dwritetextlayout_layout_GetFontStyle,
    dwritetextlayout_layout_GetFontStretch,
    dwritetextlayout_layout_GetFontSize,
    dwritetextlayout_GetUnderline,
    dwritetextlayout_GetStrikethrough,
    dwritetextlayout_GetDrawingEffect,
    dwritetextlayout_GetInlineObject,
    dwritetextlayout_GetTypography,
    dwritetextlayout_layout_GetLocaleNameLength,
    dwritetextlayout_layout_GetLocaleName,
    dwritetextlayout_Draw,
    dwritetextlayout_GetLineMetrics,
    dwritetextlayout_GetMetrics,
    dwritetextlayout_GetOverhangMetrics,
    dwritetextlayout_GetClusterMetrics,
    dwritetextlayout_DetermineMinWidth,
    dwritetextlayout_HitTestPoint,
    dwritetextlayout_HitTestTextPosition,
    dwritetextlayout_HitTestTextRange,
    dwritetextlayout1_SetPairKerning,
    dwritetextlayout1_GetPairKerning,
    dwritetextlayout1_SetCharacterSpacing,
    dwritetextlayout1_GetCharacterSpacing,
    dwritetextlayout2_GetMetrics,
    dwritetextlayout2_SetVerticalGlyphOrientation,
    dwritetextlayout2_GetVerticalGlyphOrientation,
    dwritetextlayout2_SetLastLineWrapping,
    dwritetextlayout2_GetLastLineWrapping,
    dwritetextlayout2_SetOpticalAlignment,
    dwritetextlayout2_GetOpticalAlignment,
    dwritetextlayout2_SetFontFallback,
    dwritetextlayout2_GetFontFallback
};

static HRESULT WINAPI dwritetextformat1_layout_QueryInterface(IDWriteTextFormat1 *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);
    return IDWriteTextLayout2_QueryInterface(&This->IDWriteTextLayout2_iface, riid, obj);
}

static ULONG WINAPI dwritetextformat1_layout_AddRef(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    return IDWriteTextLayout2_AddRef(&This->IDWriteTextLayout2_iface);
}

static ULONG WINAPI dwritetextformat1_layout_Release(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    return IDWriteTextLayout2_Release(&This->IDWriteTextLayout2_iface);
}

static HRESULT WINAPI dwritetextformat1_layout_SetTextAlignment(IDWriteTextFormat1 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetParagraphAlignment(IDWriteTextFormat1 *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetWordWrapping(IDWriteTextFormat1 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, wrapping);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetReadingDirection(IDWriteTextFormat1 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetFlowDirection(IDWriteTextFormat1 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetIncrementalTabStop(IDWriteTextFormat1 *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%p %p): stub\n", This, trimming, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_layout_SetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d %f %f): stub\n", This, spacing, line_spacing, baseline);
    return E_NOTIMPL;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat1_layout_GetTextAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat1_layout_GetParagraphAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat1_layout_GetWordWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return This->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat1_layout_GetReadingDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat1_layout_GetFlowDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.flow;
}

static FLOAT WINAPI dwritetextformat1_layout_GetIncrementalTabStop(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextformat1_layout_GetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %p)\n", This, options, trimming_sign);

    *options = This->format.trimming;
    *trimming_sign = This->format.trimmingsign;
    if (*trimming_sign)
        IDWriteInlineObject_AddRef(*trimming_sign);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_layout_GetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);

    *method = This->format.spacingmethod;
    *spacing = This->format.spacing;
    *baseline = This->format.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_layout_GetFontCollection(IDWriteTextFormat1 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->format.collection;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);
    return S_OK;
}

static UINT32 WINAPI dwritetextformat1_layout_GetFontFamilyNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.family_len;
}

static HRESULT WINAPI dwritetextformat1_layout_GetFontFamilyName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat1_layout_GetFontWeight(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat1_layout_GetFontStyle(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat1_layout_GetFontStretch(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.stretch;
}

static FLOAT WINAPI dwritetextformat1_layout_GetFontSize(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.fontsize;
}

static UINT32 WINAPI dwritetextformat1_layout_GetLocaleNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextformat1_layout_GetLocaleName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_layout_SetVerticalGlyphOrientation(IDWriteTextFormat1 *iface, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, orientation);
    return E_NOTIMPL;
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextformat1_layout_GetVerticalGlyphOrientation(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT;
}

static HRESULT WINAPI dwritetextformat1_layout_SetLastLineWrapping(IDWriteTextFormat1 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, lastline_wrapping_enabled);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritetextformat1_layout_GetLastLineWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritetextformat1_layout_SetOpticalAlignment(IDWriteTextFormat1 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextformat1_layout_GetOpticalAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_OPTICAL_ALIGNMENT_NONE;
}

static HRESULT WINAPI dwritetextformat1_layout_SetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return IDWriteTextLayout2_SetFontFallback(&This->IDWriteTextLayout2_iface, fallback);
}

static HRESULT WINAPI dwritetextformat1_layout_GetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textlayout *This = impl_layout_form_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return IDWriteTextLayout2_GetFontFallback(&This->IDWriteTextLayout2_iface, fallback);
}

static const IDWriteTextFormat1Vtbl dwritetextformat1_layout_vtbl = {
    dwritetextformat1_layout_QueryInterface,
    dwritetextformat1_layout_AddRef,
    dwritetextformat1_layout_Release,
    dwritetextformat1_layout_SetTextAlignment,
    dwritetextformat1_layout_SetParagraphAlignment,
    dwritetextformat1_layout_SetWordWrapping,
    dwritetextformat1_layout_SetReadingDirection,
    dwritetextformat1_layout_SetFlowDirection,
    dwritetextformat1_layout_SetIncrementalTabStop,
    dwritetextformat1_layout_SetTrimming,
    dwritetextformat1_layout_SetLineSpacing,
    dwritetextformat1_layout_GetTextAlignment,
    dwritetextformat1_layout_GetParagraphAlignment,
    dwritetextformat1_layout_GetWordWrapping,
    dwritetextformat1_layout_GetReadingDirection,
    dwritetextformat1_layout_GetFlowDirection,
    dwritetextformat1_layout_GetIncrementalTabStop,
    dwritetextformat1_layout_GetTrimming,
    dwritetextformat1_layout_GetLineSpacing,
    dwritetextformat1_layout_GetFontCollection,
    dwritetextformat1_layout_GetFontFamilyNameLength,
    dwritetextformat1_layout_GetFontFamilyName,
    dwritetextformat1_layout_GetFontWeight,
    dwritetextformat1_layout_GetFontStyle,
    dwritetextformat1_layout_GetFontStretch,
    dwritetextformat1_layout_GetFontSize,
    dwritetextformat1_layout_GetLocaleNameLength,
    dwritetextformat1_layout_GetLocaleName,
    dwritetextformat1_layout_SetVerticalGlyphOrientation,
    dwritetextformat1_layout_GetVerticalGlyphOrientation,
    dwritetextformat1_layout_SetLastLineWrapping,
    dwritetextformat1_layout_GetLastLineWrapping,
    dwritetextformat1_layout_SetOpticalAlignment,
    dwritetextformat1_layout_GetOpticalAlignment,
    dwritetextformat1_layout_SetFontFallback,
    dwritetextformat1_layout_GetFontFallback
};

static HRESULT WINAPI dwritetextlayout_sink_QueryInterface(IDWriteTextAnalysisSink *iface,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSink) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteTextAnalysisSink_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_sink_AddRef(IDWriteTextAnalysisSink *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextlayout_sink_Release(IDWriteTextAnalysisSink *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextlayout_sink_SetScriptAnalysis(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* sa)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink(iface);
    struct layout_run *run;

    TRACE("%u %u script=%d\n", position, length, sa->script);

    run = alloc_layout_run(LAYOUT_RUN_REGULAR);
    if (!run)
        return E_OUTOFMEMORY;

    run->u.regular.descr.string = &layout->str[position];
    run->u.regular.descr.stringLength = length;
    run->u.regular.descr.textPosition = position;
    run->u.regular.sa = *sa;
    list_add_tail(&layout->runs, &run->entry);
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetLineBreakpoints(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink(iface);

    if (position + length > layout->len)
        return E_FAIL;

    memcpy(&layout->nominal_breakpoints[position], breakpoints, length*sizeof(DWRITE_LINE_BREAKPOINT));
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetBidiLevel(IDWriteTextAnalysisSink *iface, UINT32 position,
    UINT32 length, UINT8 explicitLevel, UINT8 resolvedLevel)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink(iface);
    struct layout_run *cur_run;

    LIST_FOR_EACH_ENTRY(cur_run, &layout->runs, struct layout_run, entry) {
        struct regular_layout_run *cur = &cur_run->u.regular;
        struct layout_run *run, *run2;

        if (cur_run->kind == LAYOUT_RUN_INLINE)
            continue;

        /* FIXME: levels are reported in a natural forward direction, so start loop from a run we ended on */
        if (position < cur->descr.textPosition || position > cur->descr.textPosition + cur->descr.stringLength)
            continue;

        /* full hit - just set run level */
        if (cur->descr.textPosition == position && cur->descr.stringLength == length) {
            cur->run.bidiLevel = resolvedLevel;
            break;
        }

        /* current run is fully covered, move to next one */
        if (cur->descr.textPosition == position && cur->descr.stringLength < length) {
            cur->run.bidiLevel = resolvedLevel;
            position += cur->descr.stringLength;
            length -= cur->descr.stringLength;
            continue;
        }

        /* now starting point is in a run, so it splits it */
        run = alloc_layout_run(LAYOUT_RUN_REGULAR);
        if (!run)
            return E_OUTOFMEMORY;

        *run = *cur_run;
        run->u.regular.descr.textPosition = position;
        run->u.regular.descr.stringLength = cur->descr.stringLength - position + cur->descr.textPosition;
        run->u.regular.descr.string = &layout->str[position];
        run->u.regular.run.bidiLevel = resolvedLevel;
        cur->descr.stringLength -= position - cur->descr.textPosition;

        list_add_after(&cur_run->entry, &run->entry);

        if (position + length == run->u.regular.descr.textPosition + run->u.regular.descr.stringLength)
            break;

        /* split second time */
        run2 = alloc_layout_run(LAYOUT_RUN_REGULAR);
        if (!run2)
            return E_OUTOFMEMORY;

        *run2 = *cur_run;
        run2->u.regular.descr.textPosition = run->u.regular.descr.textPosition + run->u.regular.descr.stringLength;
        run2->u.regular.descr.stringLength = cur->descr.textPosition + cur->descr.stringLength - position - length;
        run2->u.regular.descr.string = &layout->str[run2->u.regular.descr.textPosition];
        run->u.regular.descr.stringLength -= run2->u.regular.descr.stringLength;

        list_add_after(&run->entry, &run2->entry);
        break;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetNumberSubstitution(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, IDWriteNumberSubstitution* substitution)
{
    return E_NOTIMPL;
}

static const IDWriteTextAnalysisSinkVtbl dwritetextlayoutsinkvtbl = {
    dwritetextlayout_sink_QueryInterface,
    dwritetextlayout_sink_AddRef,
    dwritetextlayout_sink_Release,
    dwritetextlayout_sink_SetScriptAnalysis,
    dwritetextlayout_sink_SetLineBreakpoints,
    dwritetextlayout_sink_SetBidiLevel,
    dwritetextlayout_sink_SetNumberSubstitution
};

static HRESULT WINAPI dwritetextlayout_source_QueryInterface(IDWriteTextAnalysisSource *iface,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSource) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextAnalysisSource_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_source_AddRef(IDWriteTextAnalysisSource *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextlayout_source_Release(IDWriteTextAnalysisSource *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextlayout_source_GetTextAtPosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource(iface);

    TRACE("(%p)->(%u %p %p)\n", layout, position, text, text_len);

    if (position < layout->len) {
        *text = &layout->str[position];
        *text_len = layout->len - position;
    }
    else {
        *text = NULL;
        *text_len = 0;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_source_GetTextBeforePosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    FIXME("%u %p %p: stub\n", position, text, text_len);
    return E_NOTIMPL;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_source_GetParagraphReadingDirection(IDWriteTextAnalysisSource *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource(iface);
    return IDWriteTextLayout2_GetReadingDirection(&layout->IDWriteTextLayout2_iface);
}

static HRESULT WINAPI dwritetextlayout_source_GetLocaleName(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, WCHAR const** locale)
{
    FIXME("%u %p %p: stub\n", position, text_len, locale);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_source_GetNumberSubstitution(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, IDWriteNumberSubstitution **substitution)
{
    FIXME("%u %p %p: stub\n", position, text_len, substitution);
    return E_NOTIMPL;
}

static const IDWriteTextAnalysisSourceVtbl dwritetextlayoutsourcevtbl = {
    dwritetextlayout_source_QueryInterface,
    dwritetextlayout_source_AddRef,
    dwritetextlayout_source_Release,
    dwritetextlayout_source_GetTextAtPosition,
    dwritetextlayout_source_GetTextBeforePosition,
    dwritetextlayout_source_GetParagraphReadingDirection,
    dwritetextlayout_source_GetLocaleName,
    dwritetextlayout_source_GetNumberSubstitution
};

static HRESULT layout_format_from_textformat(struct dwrite_textlayout *layout, IDWriteTextFormat *format)
{
    IDWriteTextFormat1 *format1;
    UINT32 len;
    HRESULT hr;

    layout->format.weight  = IDWriteTextFormat_GetFontWeight(format);
    layout->format.style   = IDWriteTextFormat_GetFontStyle(format);
    layout->format.stretch = IDWriteTextFormat_GetFontStretch(format);
    layout->format.fontsize= IDWriteTextFormat_GetFontSize(format);
    layout->format.textalignment = IDWriteTextFormat_GetTextAlignment(format);
    layout->format.paralign = IDWriteTextFormat_GetParagraphAlignment(format);
    layout->format.wrapping = IDWriteTextFormat_GetWordWrapping(format);
    layout->format.readingdir = IDWriteTextFormat_GetReadingDirection(format);
    layout->format.flow = IDWriteTextFormat_GetFlowDirection(format);
    layout->format.fallback = NULL;
    hr = IDWriteTextFormat_GetLineSpacing(format, &layout->format.spacingmethod,
        &layout->format.spacing, &layout->format.baseline);
    if (FAILED(hr))
        return hr;

    hr = IDWriteTextFormat_GetTrimming(format, &layout->format.trimming, &layout->format.trimmingsign);
    if (FAILED(hr))
        return hr;

    /* locale name and length */
    len = IDWriteTextFormat_GetLocaleNameLength(format);
    layout->format.locale = heap_alloc((len+1)*sizeof(WCHAR));
    if (!layout->format.locale)
        return E_OUTOFMEMORY;

    hr = IDWriteTextFormat_GetLocaleName(format, layout->format.locale, len+1);
    if (FAILED(hr))
        return hr;
    layout->format.locale_len = len;

    /* font family name and length */
    len = IDWriteTextFormat_GetFontFamilyNameLength(format);
    layout->format.family_name = heap_alloc((len+1)*sizeof(WCHAR));
    if (!layout->format.family_name)
        return E_OUTOFMEMORY;

    hr = IDWriteTextFormat_GetFontFamilyName(format, layout->format.family_name, len+1);
    if (FAILED(hr))
        return hr;
    layout->format.family_len = len;

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat1, (void**)&format1);
    if (hr == S_OK) {
        layout->format.vertical_orientation = IDWriteTextFormat1_GetVerticalGlyphOrientation(format1);
        IDWriteTextFormat1_GetFontFallback(format1, &layout->format.fallback);
        IDWriteTextFormat1_Release(format1);
    }
    else
        layout->format.vertical_orientation = DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT;

    return IDWriteTextFormat_GetFontCollection(format, &layout->format.collection);
}

static HRESULT init_textlayout(const WCHAR *str, UINT32 len, IDWriteTextFormat *format, FLOAT maxwidth, FLOAT maxheight, struct dwrite_textlayout *layout)
{
    DWRITE_TEXT_RANGE r = { 0, len };
    struct layout_range *range;
    HRESULT hr;

    layout->IDWriteTextLayout2_iface.lpVtbl = &dwritetextlayoutvtbl;
    layout->IDWriteTextFormat1_iface.lpVtbl = &dwritetextformat1_layout_vtbl;
    layout->IDWriteTextAnalysisSink_iface.lpVtbl = &dwritetextlayoutsinkvtbl;
    layout->IDWriteTextAnalysisSource_iface.lpVtbl = &dwritetextlayoutsourcevtbl;
    layout->ref = 1;
    layout->len = len;
    layout->maxwidth = maxwidth;
    layout->maxheight = maxheight;
    layout->recompute = RECOMPUTE_EVERYTHING;
    layout->nominal_breakpoints = NULL;
    layout->actual_breakpoints = NULL;
    layout->clusters_count = 0;
    layout->clusters = NULL;
    layout->minwidth = 0.0;
    list_init(&layout->runs);
    list_init(&layout->ranges);
    memset(&layout->format, 0, sizeof(layout->format));

    layout->gdicompatible = FALSE;
    layout->pixels_per_dip = 0.0;
    layout->use_gdi_natural = FALSE;
    memset(&layout->transform, 0, sizeof(layout->transform));

    layout->str = heap_strdupnW(str, len);
    if (len && !layout->str) {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    hr = layout_format_from_textformat(layout, format);
    if (FAILED(hr))
        goto fail;

    range = alloc_layout_range(layout, &r);
    if (!range) {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    list_add_head(&layout->ranges, &range->entry);
    return S_OK;

fail:
    IDWriteTextLayout2_Release(&layout->IDWriteTextLayout2_iface);
    return hr;
}

HRESULT create_textlayout(const WCHAR *str, UINT32 len, IDWriteTextFormat *format, FLOAT maxwidth, FLOAT maxheight, IDWriteTextLayout **ret)
{
    struct dwrite_textlayout *layout;
    HRESULT hr;

    *ret = NULL;

    layout = heap_alloc(sizeof(struct dwrite_textlayout));
    if (!layout) return E_OUTOFMEMORY;

    hr = init_textlayout(str, len, format, maxwidth, maxheight, layout);
    if (hr == S_OK)
        *ret = (IDWriteTextLayout*)&layout->IDWriteTextLayout2_iface;

    return hr;
}

HRESULT create_gdicompat_textlayout(const WCHAR *str, UINT32 len, IDWriteTextFormat *format, FLOAT maxwidth, FLOAT maxheight,
    FLOAT pixels_per_dip, const DWRITE_MATRIX *transform, BOOL use_gdi_natural, IDWriteTextLayout **ret)
{
    struct dwrite_textlayout *layout;
    HRESULT hr;

    *ret = NULL;

    layout = heap_alloc(sizeof(struct dwrite_textlayout));
    if (!layout) return E_OUTOFMEMORY;

    hr = init_textlayout(str, len, format, maxwidth, maxheight, layout);
    if (hr == S_OK) {
        /* set gdi-specific properties */
        layout->gdicompatible = TRUE;
        layout->pixels_per_dip = pixels_per_dip;
        layout->use_gdi_natural = use_gdi_natural;
        layout->transform = transform ? *transform : identity;

        *ret = (IDWriteTextLayout*)&layout->IDWriteTextLayout2_iface;
    }

    return hr;
}

static HRESULT WINAPI dwritetrimmingsign_QueryInterface(IDWriteInlineObject *iface, REFIID riid, void **obj)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteInlineObject)) {
        *obj = iface;
        IDWriteInlineObject_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;

}

static ULONG WINAPI dwritetrimmingsign_AddRef(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetrimmingsign_Release(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI dwritetrimmingsign_Draw(IDWriteInlineObject *iface, void *context, IDWriteTextRenderer *renderer,
    FLOAT originX, FLOAT originY, BOOL is_sideways, BOOL is_rtl, IUnknown *drawing_effect)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p %p %f %f %d %d %p): stub\n", This, context, renderer, originX, originY, is_sideways, is_rtl, drawing_effect);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetrimmingsign_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *metrics)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    memset(metrics, 0, sizeof(*metrics));
    return S_OK;
}

static HRESULT WINAPI dwritetrimmingsign_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p): stub\n", This, overhangs);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetrimmingsign_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
        DWRITE_BREAK_CONDITION *after)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);

    TRACE("(%p)->(%p %p)\n", This, before, after);

    *before = *after = DWRITE_BREAK_CONDITION_NEUTRAL;
    return S_OK;
}

static const IDWriteInlineObjectVtbl dwritetrimmingsignvtbl = {
    dwritetrimmingsign_QueryInterface,
    dwritetrimmingsign_AddRef,
    dwritetrimmingsign_Release,
    dwritetrimmingsign_Draw,
    dwritetrimmingsign_GetMetrics,
    dwritetrimmingsign_GetOverhangMetrics,
    dwritetrimmingsign_GetBreakConditions
};

HRESULT create_trimmingsign(IDWriteInlineObject **sign)
{
    struct dwrite_trimmingsign *This;

    *sign = NULL;

    This = heap_alloc(sizeof(struct dwrite_trimmingsign));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteInlineObject_iface.lpVtbl = &dwritetrimmingsignvtbl;
    This->ref = 1;

    *sign = &This->IDWriteInlineObject_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_QueryInterface(IDWriteTextFormat1 *iface, REFIID riid, void **obj)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextFormat1) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextFormat1_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextformat_AddRef(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextformat_Release(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        release_format_data(&This->format);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextformat_SetTextAlignment(IDWriteTextFormat1 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    This->format.textalignment = alignment;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetParagraphAlignment(IDWriteTextFormat1 *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    This->format.paralign = alignment;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetWordWrapping(IDWriteTextFormat1 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, wrapping);
    This->format.wrapping = wrapping;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetReadingDirection(IDWriteTextFormat1 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    This->format.readingdir = direction;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetFlowDirection(IDWriteTextFormat1 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    This->format.flow = direction;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetIncrementalTabStop(IDWriteTextFormat1 *iface, FLOAT tabstop)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p)\n", This, trimming, trimming_sign);

    This->format.trimming = *trimming;
    if (This->format.trimmingsign)
        IDWriteInlineObject_Release(This->format.trimmingsign);
    This->format.trimmingsign = trimming_sign;
    if (This->format.trimmingsign)
        IDWriteInlineObject_AddRef(This->format.trimmingsign);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD method,
    FLOAT spacing, FLOAT baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d %f %f)\n", This, method, spacing, baseline);
    This->format.spacingmethod = method;
    This->format.spacing = spacing;
    This->format.baseline = baseline;
    return S_OK;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat_GetTextAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat_GetParagraphAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat_GetWordWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat_GetReadingDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat_GetFlowDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.flow;
}

static FLOAT WINAPI dwritetextformat_GetIncrementalTabStop(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextformat_GetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p)\n", This, options, trimming_sign);

    *options = This->format.trimming;
    if ((*trimming_sign = This->format.trimmingsign))
        IDWriteInlineObject_AddRef(*trimming_sign);

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);

    *method = This->format.spacingmethod;
    *spacing = This->format.spacing;
    *baseline = This->format.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetFontCollection(IDWriteTextFormat1 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->format.collection;
    IDWriteFontCollection_AddRef(*collection);

    return S_OK;
}

static UINT32 WINAPI dwritetextformat_GetFontFamilyNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.family_len;
}

static HRESULT WINAPI dwritetextformat_GetFontFamilyName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat_GetFontWeight(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat_GetFontStyle(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat_GetFontStretch(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.stretch;
}

static FLOAT WINAPI dwritetextformat_GetFontSize(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.fontsize;
}

static UINT32 WINAPI dwritetextformat_GetLocaleNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextformat_GetLocaleName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_SetVerticalGlyphOrientation(IDWriteTextFormat1 *iface, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%d)\n", This, orientation);

    if ((UINT32)orientation > DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED)
        return E_INVALIDARG;

    This->format.vertical_orientation = orientation;
    return S_OK;
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextformat1_GetVerticalGlyphOrientation(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.vertical_orientation;
}

static HRESULT WINAPI dwritetextformat1_SetLastLineWrapping(IDWriteTextFormat1 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, lastline_wrapping_enabled);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritetextformat1_GetLastLineWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritetextformat1_SetOpticalAlignment(IDWriteTextFormat1 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextformat1_GetOpticalAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_OPTICAL_ALIGNMENT_NONE;
}

static HRESULT WINAPI dwritetextformat1_SetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return set_fontfallback_for_format(&This->format, fallback);
}

static HRESULT WINAPI dwritetextformat1_GetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p)\n", This, fallback);
    return get_fontfallback_from_format(&This->format, fallback);
}

static const IDWriteTextFormat1Vtbl dwritetextformatvtbl = {
    dwritetextformat_QueryInterface,
    dwritetextformat_AddRef,
    dwritetextformat_Release,
    dwritetextformat_SetTextAlignment,
    dwritetextformat_SetParagraphAlignment,
    dwritetextformat_SetWordWrapping,
    dwritetextformat_SetReadingDirection,
    dwritetextformat_SetFlowDirection,
    dwritetextformat_SetIncrementalTabStop,
    dwritetextformat_SetTrimming,
    dwritetextformat_SetLineSpacing,
    dwritetextformat_GetTextAlignment,
    dwritetextformat_GetParagraphAlignment,
    dwritetextformat_GetWordWrapping,
    dwritetextformat_GetReadingDirection,
    dwritetextformat_GetFlowDirection,
    dwritetextformat_GetIncrementalTabStop,
    dwritetextformat_GetTrimming,
    dwritetextformat_GetLineSpacing,
    dwritetextformat_GetFontCollection,
    dwritetextformat_GetFontFamilyNameLength,
    dwritetextformat_GetFontFamilyName,
    dwritetextformat_GetFontWeight,
    dwritetextformat_GetFontStyle,
    dwritetextformat_GetFontStretch,
    dwritetextformat_GetFontSize,
    dwritetextformat_GetLocaleNameLength,
    dwritetextformat_GetLocaleName,
    dwritetextformat1_SetVerticalGlyphOrientation,
    dwritetextformat1_GetVerticalGlyphOrientation,
    dwritetextformat1_SetLastLineWrapping,
    dwritetextformat1_GetLastLineWrapping,
    dwritetextformat1_SetOpticalAlignment,
    dwritetextformat1_GetOpticalAlignment,
    dwritetextformat1_SetFontFallback,
    dwritetextformat1_GetFontFallback
};

HRESULT create_textformat(const WCHAR *family_name, IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, const WCHAR *locale, IDWriteTextFormat **format)
{
    struct dwrite_textformat *This;

    *format = NULL;

    This = heap_alloc(sizeof(struct dwrite_textformat));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteTextFormat1_iface.lpVtbl = &dwritetextformatvtbl;
    This->ref = 1;
    This->format.family_name = heap_strdupW(family_name);
    This->format.family_len = strlenW(family_name);
    This->format.locale = heap_strdupW(locale);
    This->format.locale_len = strlenW(locale);
    This->format.weight = weight;
    This->format.style = style;
    This->format.fontsize = size;
    This->format.stretch = stretch;
    This->format.textalignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    This->format.paralign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    This->format.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    This->format.readingdir = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    This->format.flow = DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
    This->format.spacingmethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
    This->format.vertical_orientation = DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT;
    This->format.spacing = 0.0;
    This->format.baseline = 0.0;
    This->format.trimming.granularity = DWRITE_TRIMMING_GRANULARITY_NONE;
    This->format.trimming.delimiter = 0;
    This->format.trimming.delimiterCount = 0;
    This->format.trimmingsign = NULL;
    This->format.collection = collection;
    This->format.fallback = NULL;
    IDWriteFontCollection_AddRef(collection);

    *format = (IDWriteTextFormat*)&This->IDWriteTextFormat1_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetypography_QueryInterface(IDWriteTypography *iface, REFIID riid, void **obj)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%s %p)\n", typography, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTypography) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteTypography_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetypography_AddRef(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG ref = InterlockedIncrement(&typography->ref);
    TRACE("(%p)->(%d)\n", typography, ref);
    return ref;
}

static ULONG WINAPI dwritetypography_Release(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG ref = InterlockedDecrement(&typography->ref);

    TRACE("(%p)->(%d)\n", typography, ref);

    if (!ref) {
        heap_free(typography->features);
        heap_free(typography);
    }

    return ref;
}

static HRESULT WINAPI dwritetypography_AddFontFeature(IDWriteTypography *iface, DWRITE_FONT_FEATURE feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%x %u)\n", typography, feature.nameTag, feature.parameter);

    if (typography->count == typography->allocated) {
        DWRITE_FONT_FEATURE *ptr = heap_realloc(typography->features, 2*typography->allocated*sizeof(DWRITE_FONT_FEATURE));
        if (!ptr)
            return E_OUTOFMEMORY;

        typography->features = ptr;
        typography->allocated *= 2;
    }

    typography->features[typography->count++] = feature;
    return S_OK;
}

static UINT32 WINAPI dwritetypography_GetFontFeatureCount(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    TRACE("(%p)\n", typography);
    return typography->count;
}

static HRESULT WINAPI dwritetypography_GetFontFeature(IDWriteTypography *iface, UINT32 index, DWRITE_FONT_FEATURE *feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%u %p)\n", typography, index, feature);

    if (index >= typography->count)
        return E_INVALIDARG;

    *feature = typography->features[index];
    return S_OK;
}

static const IDWriteTypographyVtbl dwritetypographyvtbl = {
    dwritetypography_QueryInterface,
    dwritetypography_AddRef,
    dwritetypography_Release,
    dwritetypography_AddFontFeature,
    dwritetypography_GetFontFeatureCount,
    dwritetypography_GetFontFeature
};

HRESULT create_typography(IDWriteTypography **ret)
{
    struct dwrite_typography *typography;

    *ret = NULL;

    typography = heap_alloc(sizeof(*typography));
    if (!typography)
        return E_OUTOFMEMORY;

    typography->IDWriteTypography_iface.lpVtbl = &dwritetypographyvtbl;
    typography->ref = 1;
    typography->allocated = 2;
    typography->count = 0;

    typography->features = heap_alloc(typography->allocated*sizeof(DWRITE_FONT_FEATURE));
    if (!typography->features) {
        heap_free(typography);
        return E_OUTOFMEMORY;
    }

    *ret = &typography->IDWriteTypography_iface;
    return S_OK;
}
