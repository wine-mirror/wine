/*
 *    Font and collections
 *
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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

#include "wine/list.h"
#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define MS_HEAD_TAG DWRITE_MAKE_OPENTYPE_TAG('h','e','a','d')
#define MS_OS2_TAG  DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2')
#define MS_CMAP_TAG DWRITE_MAKE_OPENTYPE_TAG('c','m','a','p')
#define MS_NAME_TAG DWRITE_MAKE_OPENTYPE_TAG('n','a','m','e')

static const IID IID_issystemcollection = {0x14d88047,0x331f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};

struct dwrite_font_data {
    LONG ref;

    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_METRICS1 metrics;
    IDWriteLocalizedStrings *info_strings[DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME+1];

    /* data needed to create fontface instance */
    IDWriteFactory2 *factory;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFile *file;
    UINT32 face_index;

    WCHAR *facename;
};

struct dwrite_fontfamily_data {
    LONG ref;

    IDWriteLocalizedStrings *familyname;

    struct dwrite_font_data **fonts;
    UINT32 font_count;
    UINT32 font_alloc;
};

struct dwrite_fontcollection {
    IDWriteFontCollection IDWriteFontCollection_iface;
    LONG ref;

    struct dwrite_fontfamily_data **family_data;
    UINT32 family_count;
    UINT32 family_alloc;
    BOOL   is_system;
};

struct dwrite_fontfamily {
    IDWriteFontFamily IDWriteFontFamily_iface;
    LONG ref;

    struct dwrite_fontfamily_data *data;

    IDWriteFontCollection* collection;
};

struct dwrite_font {
    IDWriteFont2 IDWriteFont2_iface;
    LONG ref;

    IDWriteFontFamily *family;

    USHORT simulations;
    DWRITE_FONT_STYLE style;
    struct dwrite_font_data *data;
};

struct dwrite_fonttable {
    void  *data;
    void  *context;
    UINT32 size;
};

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

struct dwrite_fontface {
    IDWriteFontFace2 IDWriteFontFace2_iface;
    LONG ref;

    IDWriteFontFileStream **streams;
    IDWriteFontFile **files;
    UINT32 file_count;
    UINT32 index;

    USHORT simulations;
    DWRITE_FONT_FACE_TYPE type;
    DWRITE_FONT_METRICS1 metrics;
    DWRITE_CARET_METRICS caret;

    struct dwrite_fonttable cmap;
    DWRITE_GLYPH_METRICS *glyphs[GLYPH_MAX/GLYPH_BLOCK_SIZE];
};

struct dwrite_fontfile {
    IDWriteFontFile IDWriteFontFile_iface;
    LONG ref;

    IDWriteFontFileLoader *loader;
    void *reference_key;
    UINT32 key_size;
    IDWriteFontFileStream *stream;
};

static inline struct dwrite_fontface *impl_from_IDWriteFontFace2(IDWriteFontFace2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontface, IDWriteFontFace2_iface);
}

static inline struct dwrite_font *impl_from_IDWriteFont2(IDWriteFont2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_font, IDWriteFont2_iface);
}

static inline struct dwrite_fontfile *impl_from_IDWriteFontFile(IDWriteFontFile *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfile, IDWriteFontFile_iface);
}

static inline struct dwrite_fontfamily *impl_from_IDWriteFontFamily(IDWriteFontFamily *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfamily, IDWriteFontFamily_iface);
}

static inline struct dwrite_fontcollection *impl_from_IDWriteFontCollection(IDWriteFontCollection *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontcollection, IDWriteFontCollection_iface);
}

static HRESULT get_cached_glyph_metrics(struct dwrite_fontface *fontface, UINT16 glyph, DWRITE_GLYPH_METRICS *metrics)
{
    static const DWRITE_GLYPH_METRICS nil;
    DWRITE_GLYPH_METRICS *block = fontface->glyphs[glyph >> GLYPH_BLOCK_SHIFT];

    if (!block || !memcmp(&block[glyph & GLYPH_BLOCK_MASK], &nil, sizeof(DWRITE_GLYPH_METRICS))) return S_FALSE;
    memcpy(metrics, &block[glyph & GLYPH_BLOCK_MASK], sizeof(*metrics));
    return S_OK;
}

static HRESULT set_cached_glyph_metrics(struct dwrite_fontface *fontface, UINT16 glyph, DWRITE_GLYPH_METRICS *metrics)
{
    DWRITE_GLYPH_METRICS **block = &fontface->glyphs[glyph >> GLYPH_BLOCK_SHIFT];

    if (!*block) {
        /* start new block */
        *block = heap_alloc_zero(sizeof(*metrics) * GLYPH_BLOCK_SIZE);
        if (!*block)
            return E_OUTOFMEMORY;
    }

    memcpy(&(*block)[glyph & GLYPH_BLOCK_MASK], metrics, sizeof(*metrics));
    return S_OK;
}

static inline void* get_fontface_cmap(struct dwrite_fontface *fontface)
{
    BOOL exists = FALSE;
    HRESULT hr;

    if (fontface->cmap.data)
        return fontface->cmap.data;

    hr = IDWriteFontFace2_TryGetFontTable(&fontface->IDWriteFontFace2_iface, MS_CMAP_TAG, (const void**)&fontface->cmap.data,
        &fontface->cmap.size, &fontface->cmap.context, &exists);
    if (FAILED(hr) || !exists) {
        ERR("Font does not have a CMAP table\n");
        return NULL;
    }

    return fontface->cmap.data;
}

static void release_font_data(struct dwrite_font_data *data)
{
    int i;

    if (InterlockedDecrement(&data->ref) > 0)
        return;

    for (i = DWRITE_INFORMATIONAL_STRING_NONE; i < sizeof(data->info_strings)/sizeof(data->info_strings[0]); i++) {
        if (data->info_strings[i])
            IDWriteLocalizedStrings_Release(data->info_strings[i]);
    }

    IDWriteFontFile_Release(data->file);
    IDWriteFactory2_Release(data->factory);
    heap_free(data->facename);
    heap_free(data);
}

static void release_fontfamily_data(struct dwrite_fontfamily_data *data)
{
    int i;

    if (InterlockedDecrement(&data->ref) > 0)
        return;

    for (i = 0; i < data->font_count; i++)
        release_font_data(data->fonts[i]);
    heap_free(data->fonts);
    IDWriteLocalizedStrings_Release(data->familyname);
    heap_free(data);
}

static HRESULT WINAPI dwritefontface_QueryInterface(IDWriteFontFace2 *iface, REFIID riid, void **obj)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFace2) ||
        IsEqualIID(riid, &IID_IDWriteFontFace1) ||
        IsEqualIID(riid, &IID_IDWriteFontFace) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontFace2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontface_AddRef(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontface_Release(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        UINT32 i;

        if (This->cmap.context)
            IDWriteFontFace2_ReleaseFontTable(iface, This->cmap.context);
        for (i = 0; i < This->file_count; i++) {
            if (This->streams[i])
                IDWriteFontFileStream_Release(This->streams[i]);
            if (This->files[i])
                IDWriteFontFile_Release(This->files[i]);
        }

        for (i = 0; i < sizeof(This->glyphs)/sizeof(This->glyphs[0]); i++)
            heap_free(This->glyphs[i]);

        freetype_notify_cacheremove(iface);
        heap_free(This);
    }

    return ref;
}

static DWRITE_FONT_FACE_TYPE WINAPI dwritefontface_GetType(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)\n", This);
    return This->type;
}

static HRESULT WINAPI dwritefontface_GetFiles(IDWriteFontFace2 *iface, UINT32 *number_of_files,
    IDWriteFontFile **fontfiles)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    int i;

    TRACE("(%p)->(%p %p)\n", This, number_of_files, fontfiles);
    if (fontfiles == NULL)
    {
        *number_of_files = This->file_count;
        return S_OK;
    }
    if (*number_of_files < This->file_count)
        return E_INVALIDARG;

    for (i = 0; i < This->file_count; i++)
    {
        IDWriteFontFile_AddRef(This->files[i]);
        fontfiles[i] = This->files[i];
    }

    return S_OK;
}

static UINT32 WINAPI dwritefontface_GetIndex(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)\n", This);
    return This->index;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefontface_GetSimulations(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)\n", This);
    return This->simulations;
}

static BOOL WINAPI dwritefontface_IsSymbolFont(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static void WINAPI dwritefontface_GetMetrics(IDWriteFontFace2 *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)->(%p)\n", This, metrics);
    memcpy(metrics, &This->metrics, sizeof(*metrics));
}

static UINT16 WINAPI dwritefontface_GetGlyphCount(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)\n", This);
    return freetype_get_glyphcount(iface);
}

static HRESULT WINAPI dwritefontface_GetDesignGlyphMetrics(IDWriteFontFace2 *iface,
    UINT16 const *glyphs, UINT32 glyph_count, DWRITE_GLYPH_METRICS *ret, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    HRESULT hr;
    UINT32 i;

    TRACE("(%p)->(%p %u %p %d)\n", This, glyphs, glyph_count, ret, is_sideways);

    if (!glyphs)
        return E_INVALIDARG;

    if (is_sideways)
        FIXME("sideways metrics are not supported.\n");

    for (i = 0; i < glyph_count; i++) {
        DWRITE_GLYPH_METRICS metrics;

        hr = get_cached_glyph_metrics(This, glyphs[i], &metrics);
        if (hr != S_OK) {
            freetype_get_design_glyph_metrics(iface, This->metrics.designUnitsPerEm, glyphs[i], &metrics);
            hr = set_cached_glyph_metrics(This, glyphs[i], &metrics);
            if (FAILED(hr))
                return hr;
        }
        ret[i] = metrics;
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontface_GetGlyphIndices(IDWriteFontFace2 *iface, UINT32 const *codepoints,
    UINT32 count, UINT16 *glyph_indices)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    unsigned int i;
    void *data;

    TRACE("(%p)->(%p %u %p)\n", This, codepoints, count, glyph_indices);

    data = get_fontface_cmap(This);
    if (!data)
        return E_FAIL;

    for (i = 0; i < count; i++)
        opentype_cmap_get_glyphindex(data, codepoints[i], &glyph_indices[i]);

    return S_OK;
}

static HRESULT WINAPI dwritefontface_TryGetFontTable(IDWriteFontFace2 *iface, UINT32 table_tag,
    const void **table_data, UINT32 *table_size, void **context, BOOL *exists)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);

    TRACE("(%p)->(%u %p %p %p %p)\n", This, table_tag, table_data, table_size, context, exists);

    return opentype_get_font_table(This->streams[0], This->type, This->index, table_tag, table_data, context, table_size, exists);
}

static void WINAPI dwritefontface_ReleaseFontTable(IDWriteFontFace2 *iface, void *table_context)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);

    TRACE("(%p)->(%p)\n", This, table_context);

    IDWriteFontFileStream_ReleaseFileFragment(This->streams[0], table_context);
}

HRESULT new_glyph_outline(UINT32 count, struct glyph_outline **ret)
{
    struct glyph_outline *outline;
    D2D1_POINT_2F *points;
    UINT8 *tags;

    *ret = NULL;

    outline = heap_alloc(sizeof(*outline));
    if (!outline)
        return E_OUTOFMEMORY;

    points = heap_alloc(count*sizeof(D2D1_POINT_2F));
    tags = heap_alloc(count*sizeof(UINT8));
    if (!points || !tags) {
        heap_free(points);
        heap_free(tags);
        heap_free(outline);
        return E_OUTOFMEMORY;
    }

    outline->points = points;
    outline->tags = tags;
    outline->count = count;
    outline->advance = 0.0;

    *ret = outline;
    return S_OK;
}

static void free_glyph_outline(struct glyph_outline *outline)
{
    heap_free(outline->points);
    heap_free(outline->tags);
    heap_free(outline);
}

static void report_glyph_outline(const struct glyph_outline *outline, IDWriteGeometrySink *sink)
{
    UINT16 p;

    for (p = 0; p < outline->count; p++) {
        if (outline->tags[p] & OUTLINE_POINT_START) {
            ID2D1SimplifiedGeometrySink_BeginFigure(sink, outline->points[p], D2D1_FIGURE_BEGIN_FILLED);
            continue;
        }

        if (outline->tags[p] & OUTLINE_POINT_LINE)
            ID2D1SimplifiedGeometrySink_AddLines(sink, outline->points+p, 1);
        else if (outline->tags[p] & OUTLINE_POINT_BEZIER) {
            static const UINT16 segment_length = 3;
            ID2D1SimplifiedGeometrySink_AddBeziers(sink, (D2D1_BEZIER_SEGMENT*)&outline->points[p], 1);
            p += segment_length - 1;
        }

        if (outline->tags[p] & OUTLINE_POINT_END)
            ID2D1SimplifiedGeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    }
}

static inline void translate_glyph_outline(struct glyph_outline *outline, FLOAT xoffset, FLOAT yoffset)
{
    UINT16 p;

    for (p = 0; p < outline->count; p++) {
        outline->points[p].x += xoffset;
        outline->points[p].y += yoffset;
    }
}

static HRESULT WINAPI dwritefontface_GetGlyphRunOutline(IDWriteFontFace2 *iface, FLOAT emSize,
    UINT16 const *glyphs, FLOAT const* advances, DWRITE_GLYPH_OFFSET const *offsets,
    UINT32 count, BOOL is_sideways, BOOL is_rtl, IDWriteGeometrySink *sink)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FLOAT advance = 0.0;
    HRESULT hr;
    UINT32 g;

    TRACE("(%p)->(%.2f %p %p %p %u %d %d %p)\n", This, emSize, glyphs, advances, offsets,
        count, is_sideways, is_rtl, sink);

    if (!glyphs || !sink)
        return E_INVALIDARG;

    if (is_sideways)
        FIXME("sideways mode is not supported.\n");

    for (g = 0; g < count; g++) {
        FLOAT xoffset = 0.0, yoffset = 0.0;
        struct glyph_outline *outline;

        /* FIXME: cache outlines */

        hr = freetype_get_glyph_outline(iface, emSize, glyphs[g], This->simulations, &outline);
        if (FAILED(hr))
            return hr;

        /* glyph offsets act as current glyph adjustment */
        if (offsets) {
            xoffset += is_rtl ? -offsets[g].advanceOffset : offsets[g].advanceOffset;
            yoffset -= offsets[g].ascenderOffset;
        }

        if (g == 0)
            advance = is_rtl ? -outline->advance : 0.0;

        xoffset += advance;
        translate_glyph_outline(outline, xoffset, yoffset);

        /* update advance to next glyph */
        if (advances)
            advance += is_rtl ? -advances[g] : advances[g];
        else
            advance += is_rtl ? -outline->advance : outline->advance;

        report_glyph_outline(outline, sink);
        free_glyph_outline(outline);
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontface_GetRecommendedRenderingMode(IDWriteFontFace2 *iface, FLOAT emSize,
    FLOAT pixels_per_dip, DWRITE_MEASURING_MODE mode, IDWriteRenderingParams* params, DWRITE_RENDERING_MODE* rendering_mode)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %d %p %p): stub\n", This, emSize, pixels_per_dip, mode, params, rendering_mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleMetrics(IDWriteFontFace2 *iface, FLOAT emSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const *transform, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    DWRITE_FONT_METRICS1 metrics1;
    HRESULT hr;

    TRACE("(%p)->(%.2f %.2f %p %p)\n", This, emSize, pixels_per_dip, transform, metrics);

    hr = IDWriteFontFace2_GetGdiCompatibleMetrics(iface, emSize, pixels_per_dip, transform, &metrics1);
    if (FAILED(hr))
        return hr;

    memcpy(metrics, &metrics1, sizeof(*metrics));
    return hr;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleGlyphMetrics(IDWriteFontFace2 *iface, FLOAT emSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const *transform, BOOL use_gdi_natural, UINT16 const *glyph_indices, UINT32 glyph_count,
    DWRITE_GLYPH_METRICS *metrics, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %p %d %p %u %p %d): stub\n", This, emSize, pixels_per_dip, transform, use_gdi_natural, glyph_indices,
        glyph_count, metrics, is_sideways);
    return E_NOTIMPL;
}

static void WINAPI dwritefontface1_GetMetrics(IDWriteFontFace2 *iface, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)->(%p)\n", This, metrics);
    *metrics = This->metrics;
}

static HRESULT WINAPI dwritefontface1_GetGdiCompatibleMetrics(IDWriteFontFace2 *iface, FLOAT em_size, FLOAT pixels_per_dip,
    const DWRITE_MATRIX *transform, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %p %p): stub\n", This, em_size, pixels_per_dip, transform, metrics);
    return E_NOTIMPL;
}

static void WINAPI dwritefontface1_GetCaretMetrics(IDWriteFontFace2 *iface, DWRITE_CARET_METRICS *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)->(%p)\n", This, metrics);
    *metrics = This->caret;
}

static HRESULT WINAPI dwritefontface1_GetUnicodeRanges(IDWriteFontFace2 *iface, UINT32 max_count,
    DWRITE_UNICODE_RANGE *ranges, UINT32 *count)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);

    TRACE("(%p)->(%u %p %p)\n", This, max_count, ranges, count);

    *count = 0;
    if (max_count && !ranges)
        return E_INVALIDARG;

    return opentype_cmap_get_unicode_ranges(get_fontface_cmap(This), max_count, ranges, count);
}

static BOOL WINAPI dwritefontface1_IsMonospacedFont(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    TRACE("(%p)\n", This);
    return freetype_is_monospaced(iface);
}

static HRESULT WINAPI dwritefontface1_GetDesignGlyphAdvances(IDWriteFontFace2 *iface,
    UINT32 glyph_count, UINT16 const *glyphs, INT32 *advances, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    UINT32 i;

    TRACE("(%p)->(%u %p %p %d)\n", This, glyph_count, glyphs, advances, is_sideways);

    for (i = 0; i < glyph_count; i++) {
        DWRITE_GLYPH_METRICS metrics = { 0 };
        HRESULT hr;

        hr = IDWriteFontFace2_GetDesignGlyphMetrics(iface, glyphs + i, 1, &metrics, is_sideways);
        if (FAILED(hr))
            return hr;

        advances[i] = is_sideways ? metrics.advanceHeight : metrics.advanceWidth;
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontface1_GetGdiCompatibleGlyphAdvances(IDWriteFontFace2 *iface,
    FLOAT em_size, FLOAT pixels_per_dip, const DWRITE_MATRIX *transform, BOOL use_gdi_natural,
    BOOL is_sideways, UINT32 glyph_count, UINT16 const *indices, INT32 *advances)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %p %d %d %u %p %p): stub\n", This, em_size, pixels_per_dip, transform,
        use_gdi_natural, is_sideways, glyph_count, indices, advances);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface1_GetKerningPairAdjustments(IDWriteFontFace2 *iface, UINT32 glyph_count,
    const UINT16 *indices, INT32 *adjustments)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, glyph_count, indices, adjustments);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritefontface1_HasKerningPairs(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritefontface1_GetRecommendedRenderingMode(IDWriteFontFace2 *iface,
    FLOAT font_emsize, FLOAT dpiX, FLOAT dpiY, const DWRITE_MATRIX *transform, BOOL is_sideways,
    DWRITE_OUTLINE_THRESHOLD threshold, DWRITE_MEASURING_MODE measuring_mode, DWRITE_RENDERING_MODE *rendering_mode)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %f %p %d %d %d %p): stub\n", This, font_emsize, dpiX, dpiY, transform, is_sideways,
        threshold, measuring_mode, rendering_mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface1_GetVerticalGlyphVariants(IDWriteFontFace2 *iface, UINT32 glyph_count,
    const UINT16 *nominal_indices, UINT16 *vertical_indices)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, glyph_count, nominal_indices, vertical_indices);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritefontface1_HasVerticalGlyphVariants(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static BOOL WINAPI dwritefontface2_IsColorFont(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static UINT32 WINAPI dwritefontface2_GetColorPaletteCount(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static UINT32 WINAPI dwritefontface2_GetPaletteEntryCount(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static HRESULT WINAPI dwritefontface2_GetPaletteEntries(IDWriteFontFace2 *iface, UINT32 palette_index,
    UINT32 first_entry_index, UINT32 entry_count, DWRITE_COLOR_F *entries)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%u %u %u %p): stub\n", This, palette_index, first_entry_index, entry_count, entries);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface2_GetRecommendedRenderingMode(IDWriteFontFace2 *iface, FLOAT fontEmSize,
    FLOAT dpiX, FLOAT dpiY, DWRITE_MATRIX const *transform, BOOL is_sideways, DWRITE_OUTLINE_THRESHOLD threshold,
    DWRITE_MEASURING_MODE measuringmode, IDWriteRenderingParams *params, DWRITE_RENDERING_MODE *renderingmode,
    DWRITE_GRID_FIT_MODE *gridfitmode)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %f %f %p %d %d %d %p %p %p): stub\n", This, fontEmSize, dpiX, dpiY, transform, is_sideways, threshold,
        measuringmode, params, renderingmode, gridfitmode);
    return E_NOTIMPL;
}

static const IDWriteFontFace2Vtbl dwritefontfacevtbl = {
    dwritefontface_QueryInterface,
    dwritefontface_AddRef,
    dwritefontface_Release,
    dwritefontface_GetType,
    dwritefontface_GetFiles,
    dwritefontface_GetIndex,
    dwritefontface_GetSimulations,
    dwritefontface_IsSymbolFont,
    dwritefontface_GetMetrics,
    dwritefontface_GetGlyphCount,
    dwritefontface_GetDesignGlyphMetrics,
    dwritefontface_GetGlyphIndices,
    dwritefontface_TryGetFontTable,
    dwritefontface_ReleaseFontTable,
    dwritefontface_GetGlyphRunOutline,
    dwritefontface_GetRecommendedRenderingMode,
    dwritefontface_GetGdiCompatibleMetrics,
    dwritefontface_GetGdiCompatibleGlyphMetrics,
    dwritefontface1_GetMetrics,
    dwritefontface1_GetGdiCompatibleMetrics,
    dwritefontface1_GetCaretMetrics,
    dwritefontface1_GetUnicodeRanges,
    dwritefontface1_IsMonospacedFont,
    dwritefontface1_GetDesignGlyphAdvances,
    dwritefontface1_GetGdiCompatibleGlyphAdvances,
    dwritefontface1_GetKerningPairAdjustments,
    dwritefontface1_HasKerningPairs,
    dwritefontface1_GetRecommendedRenderingMode,
    dwritefontface1_GetVerticalGlyphVariants,
    dwritefontface1_HasVerticalGlyphVariants,
    dwritefontface2_IsColorFont,
    dwritefontface2_GetColorPaletteCount,
    dwritefontface2_GetPaletteEntryCount,
    dwritefontface2_GetPaletteEntries,
    dwritefontface2_GetRecommendedRenderingMode
};

HRESULT convert_fontface_to_logfont(IDWriteFontFace *face, LOGFONTW *logfont)
{
    DWRITE_FONT_SIMULATIONS simulations;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFileStream *stream;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_WEIGHT weight;
    IDWriteFontFile *file = NULL;
    UINT32 index;
    HRESULT hr;

    memset(logfont, 0, sizeof(*logfont));

    index = 1;
    hr = IDWriteFontFace_GetFiles(face, &index, &file);
    if (FAILED(hr) || !file)
        return hr;

    hr = get_filestream_from_file(file, &stream);
    if (FAILED(hr)) {
        IDWriteFontFile_Release(file);
        return hr;
    }

    index = IDWriteFontFace_GetIndex(face);
    face_type = IDWriteFontFace_GetType(face);
    opentype_get_font_properties(stream, face_type, index, &stretch, &weight, &style);
    IDWriteFontFileStream_Release(stream);

    simulations = IDWriteFontFace_GetSimulations(face);

    logfont->lfCharSet = DEFAULT_CHARSET;
    logfont->lfWeight = weight;
    logfont->lfItalic = style == DWRITE_FONT_STYLE_ITALIC || (simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE);
    logfont->lfOutPrecision = OUT_OUTLINE_PRECIS;
    /* TODO: set facename */

    return S_OK;
}

static HRESULT get_fontface_from_font(struct dwrite_font *font, IDWriteFontFace2 **fontface)
{
    struct dwrite_font_data *data = font->data;
    IDWriteFontFace *face;
    HRESULT hr;

    *fontface = NULL;

    hr = IDWriteFactory2_CreateFontFace(data->factory, data->face_type, 1, &data->file,
            data->face_index, font->simulations, &face);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFace_QueryInterface(face, &IID_IDWriteFontFace2, (void**)fontface);
    IDWriteFontFace_Release(face);

    return hr;
}

static HRESULT WINAPI dwritefont_QueryInterface(IDWriteFont2 *iface, REFIID riid, void **obj)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFont2) ||
        IsEqualIID(riid, &IID_IDWriteFont1) ||
        IsEqualIID(riid, &IID_IDWriteFont)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFont2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefont_AddRef(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefont_Release(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        IDWriteFontFamily_Release(This->family);
        release_font_data(This->data);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritefont_GetFontFamily(IDWriteFont2 *iface, IDWriteFontFamily **family)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)->(%p)\n", This, family);

    *family = This->family;
    IDWriteFontFamily_AddRef(*family);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritefont_GetWeight(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)\n", This);
    return This->data->weight;
}

static DWRITE_FONT_STRETCH WINAPI dwritefont_GetStretch(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)\n", This);
    return This->data->stretch;
}

static DWRITE_FONT_STYLE WINAPI dwritefont_GetStyle(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)\n", This);
    return This->style;
}

static BOOL WINAPI dwritefont_IsSymbolFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteFontFace2 *fontface;
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = get_fontface_from_font(This, &fontface);
    if (FAILED(hr))
        return hr;

    return IDWriteFontFace2_IsSymbolFont(fontface);
}

static HRESULT WINAPI dwritefont_GetFaceNames(IDWriteFont2 *iface, IDWriteLocalizedStrings **names)
{
    static const WCHAR boldobliqueW[] = {'B','o','l','d',' ','O','b','l','i','q','u','e',0};
    static const WCHAR obliqueW[] = {'O','b','l','i','q','u','e',0};
    static const WCHAR boldW[] = {'B','o','l','d',0};
    static const WCHAR enusW[] = {'e','n','-','u','s',0};

    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteLocalizedStrings *strings;
    const WCHAR *name;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, names);

    *names = NULL;

    if (This->simulations == DWRITE_FONT_SIMULATIONS_NONE) {
        BOOL exists;
        return IDWriteFont2_GetInformationalStrings(iface, DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES,
            names, &exists);
    }

    switch (This->simulations) {
    case DWRITE_FONT_SIMULATIONS_BOLD|DWRITE_FONT_SIMULATIONS_OBLIQUE:
        name = boldobliqueW;
        break;
    case DWRITE_FONT_SIMULATIONS_BOLD:
        name = boldW;
        break;
    case DWRITE_FONT_SIMULATIONS_OBLIQUE:
        name = obliqueW;
        break;
    default:
        ERR("unknown simulations %d\n", This->simulations);
        return E_FAIL;
    }

    hr = create_localizedstrings(&strings);
    if (FAILED(hr)) return hr;

    hr = add_localizedstring(strings, enusW, name);
    if (FAILED(hr)) {
        IDWriteLocalizedStrings_Release(strings);
        return hr;
    }

    *names = strings;

    return S_OK;
}

static HRESULT WINAPI dwritefont_GetInformationalStrings(IDWriteFont2 *iface,
    DWRITE_INFORMATIONAL_STRING_ID stringid, IDWriteLocalizedStrings **strings, BOOL *exists)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    struct dwrite_font_data *data = This->data;
    HRESULT hr;

    TRACE("(%p)->(%d %p %p)\n", This, stringid, strings, exists);

    *exists = FALSE;
    *strings = NULL;

    if (stringid > DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME || stringid == DWRITE_INFORMATIONAL_STRING_NONE)
        return S_OK;

    if (!data->info_strings[stringid]) {
        IDWriteFontFace2 *fontface;
        const void *table_data;
        BOOL table_exists;
        void *context;
        UINT32 size;

        hr = get_fontface_from_font(This, &fontface);
        if (FAILED(hr))
            return hr;

        table_exists = FALSE;
        hr = IDWriteFontFace2_TryGetFontTable(fontface, MS_NAME_TAG, &table_data, &size, &context, &table_exists);
        if (FAILED(hr) || !table_exists)
            WARN("no NAME table found.\n");

        if (table_exists) {
            hr = opentype_get_font_strings_from_id(table_data, stringid, &data->info_strings[stringid]);
            if (FAILED(hr) || !data->info_strings[stringid])
                return hr;
            IDWriteFontFace2_ReleaseFontTable(fontface, context);
        }
    }

    hr = clone_localizedstring(data->info_strings[stringid], strings);
    if (FAILED(hr))
        return hr;

    *exists = TRUE;
    return S_OK;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefont_GetSimulations(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)\n", This);
    return This->simulations;
}

static void WINAPI dwritefont_GetMetrics(IDWriteFont2 *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);

    TRACE("(%p)->(%p)\n", This, metrics);
    memcpy(metrics, &This->data->metrics, sizeof(*metrics));
}

static HRESULT WINAPI dwritefont_HasCharacter(IDWriteFont2 *iface, UINT32 value, BOOL *exists)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteFontFace2 *fontface;
    UINT16 index;
    HRESULT hr;

    TRACE("(%p)->(0x%08x %p)\n", This, value, exists);

    *exists = FALSE;

    hr = get_fontface_from_font(This, &fontface);
    if (FAILED(hr))
        return hr;

    index = 0;
    hr = IDWriteFontFace2_GetGlyphIndices(fontface, &value, 1, &index);
    if (FAILED(hr))
        return hr;

    *exists = index != 0;
    return S_OK;
}

static HRESULT WINAPI dwritefont_CreateFontFace(IDWriteFont2 *iface, IDWriteFontFace **face)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, face);

    hr = get_fontface_from_font(This, (IDWriteFontFace2**)face);
    if (hr == S_OK)
        IDWriteFontFace_AddRef(*face);

    return hr;
}

static void WINAPI dwritefont1_GetMetrics(IDWriteFont2 *iface, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    TRACE("(%p)->(%p)\n", This, metrics);
    *metrics = This->data->metrics;
}

static void WINAPI dwritefont1_GetPanose(IDWriteFont2 *iface, DWRITE_PANOSE *panose)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    FIXME("(%p)->(%p): stub\n", This, panose);
}

static HRESULT WINAPI dwritefont1_GetUnicodeRanges(IDWriteFont2 *iface, UINT32 max_count, DWRITE_UNICODE_RANGE *ranges, UINT32 *count)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteFontFace2 *fontface;
    HRESULT hr;

    TRACE("(%p)->(%u %p %p)\n", This, max_count, ranges, count);

    hr = get_fontface_from_font(This, &fontface);
    if (FAILED(hr))
        return hr;

    return IDWriteFontFace2_GetUnicodeRanges(fontface, max_count, ranges, count);
}

static BOOL WINAPI dwritefont1_IsMonospacedFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteFontFace2 *fontface;
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = get_fontface_from_font(This, &fontface);
    if (FAILED(hr))
        return hr;

    return IDWriteFontFace2_IsMonospacedFont(fontface);
}

static HRESULT WINAPI dwritefont2_IsColorFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    IDWriteFontFace2 *fontface;
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = get_fontface_from_font(This, &fontface);
    if (FAILED(hr))
        return hr;

    return IDWriteFontFace2_IsColorFont(fontface);
}

static const IDWriteFont2Vtbl dwritefontvtbl = {
    dwritefont_QueryInterface,
    dwritefont_AddRef,
    dwritefont_Release,
    dwritefont_GetFontFamily,
    dwritefont_GetWeight,
    dwritefont_GetStretch,
    dwritefont_GetStyle,
    dwritefont_IsSymbolFont,
    dwritefont_GetFaceNames,
    dwritefont_GetInformationalStrings,
    dwritefont_GetSimulations,
    dwritefont_GetMetrics,
    dwritefont_HasCharacter,
    dwritefont_CreateFontFace,
    dwritefont1_GetMetrics,
    dwritefont1_GetPanose,
    dwritefont1_GetUnicodeRanges,
    dwritefont1_IsMonospacedFont,
    dwritefont2_IsColorFont
};

static HRESULT create_font(struct dwrite_font_data *data, IDWriteFontFamily *family, DWRITE_FONT_SIMULATIONS simulations,
    IDWriteFont **font)
{
    struct dwrite_font *This;
    *font = NULL;

    This = heap_alloc(sizeof(struct dwrite_font));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFont2_iface.lpVtbl = &dwritefontvtbl;
    This->ref = 1;
    This->family = family;
    IDWriteFontFamily_AddRef(family);
    This->simulations = simulations;
    This->style = data->style;
    This->data = data;
    InterlockedIncrement(&This->data->ref);

    /* set oblique style from requested simulation */
    if ((simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE) && data->style == DWRITE_FONT_STYLE_NORMAL)
        This->style = DWRITE_FONT_STYLE_OBLIQUE;

    *font = (IDWriteFont*)&This->IDWriteFont2_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontfamily_QueryInterface(IDWriteFontFamily *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFontList) ||
        IsEqualIID(riid, &IID_IDWriteFontFamily))
    {
        *obj = iface;
        IDWriteFontFamily_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontfamily_AddRef(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontfamily_Release(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDWriteFontCollection_Release(This->collection);
        release_fontfamily_data(This->data);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritefontfamily_GetFontCollection(IDWriteFontFamily *iface, IDWriteFontCollection **collection)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->collection;
    IDWriteFontCollection_AddRef(This->collection);
    return S_OK;
}

static UINT32 WINAPI dwritefontfamily_GetFontCount(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    TRACE("(%p)\n", This);
    return This->data->font_count;
}

static HRESULT WINAPI dwritefontfamily_GetFont(IDWriteFontFamily *iface, UINT32 index, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);

    TRACE("(%p)->(%u %p)\n", This, index, font);

    *font = NULL;

    if (This->data->font_count == 0)
        return S_FALSE;

    if (index >= This->data->font_count)
        return E_INVALIDARG;

    return create_font(This->data->fonts[index], iface, DWRITE_FONT_SIMULATIONS_NONE, font);
}

static HRESULT WINAPI dwritefontfamily_GetFamilyNames(IDWriteFontFamily *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    return clone_localizedstring(This->data->familyname, names);
}

static inline BOOL is_matching_font_style(DWRITE_FONT_STYLE style, DWRITE_FONT_STYLE font_style)
{
    if (style == font_style)
        return TRUE;

    if (((style == DWRITE_FONT_STYLE_ITALIC) || (style == DWRITE_FONT_STYLE_OBLIQUE)) && font_style == DWRITE_FONT_STYLE_NORMAL)
        return TRUE;

    return FALSE;
}

static HRESULT WINAPI dwritefontfamily_GetFirstMatchingFont(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    UINT32 min_weight_diff = ~0u;
    int found = -1, i;

    TRACE("(%p)->(%d %d %d %p)\n", This, weight, stretch, style, font);

    for (i = 0; i < This->data->font_count; i++) {
        if (is_matching_font_style(style, This->data->fonts[i]->style) && stretch == This->data->fonts[i]->stretch) {
            DWRITE_FONT_WEIGHT font_weight = This->data->fonts[i]->weight;
            UINT32 weight_diff = abs(font_weight - weight);
            if (weight_diff < min_weight_diff) {
                min_weight_diff = weight_diff;
                found = i;
            }
        }
    }

    if (found != -1) {
        DWRITE_FONT_SIMULATIONS simulations = DWRITE_FONT_SIMULATIONS_NONE;

        if (((style == DWRITE_FONT_STYLE_ITALIC) || (style == DWRITE_FONT_STYLE_OBLIQUE)) &&
            This->data->fonts[found]->style == DWRITE_FONT_STYLE_NORMAL) {
            simulations = DWRITE_FONT_SIMULATIONS_OBLIQUE;
        }
        return create_font(This->data->fonts[found], iface, simulations, font);
    }
    else {
        *font = NULL;
        return DWRITE_E_NOFONT;
    }
}

static HRESULT WINAPI dwritefontfamily_GetMatchingFonts(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFontList **fonts)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%d %d %d %p): stub\n", This, weight, stretch, style, fonts);
    return E_NOTIMPL;
}

static const IDWriteFontFamilyVtbl fontfamilyvtbl = {
    dwritefontfamily_QueryInterface,
    dwritefontfamily_AddRef,
    dwritefontfamily_Release,
    dwritefontfamily_GetFontCollection,
    dwritefontfamily_GetFontCount,
    dwritefontfamily_GetFont,
    dwritefontfamily_GetFamilyNames,
    dwritefontfamily_GetFirstMatchingFont,
    dwritefontfamily_GetMatchingFonts
};

static HRESULT create_fontfamily(struct dwrite_fontfamily_data *data, IDWriteFontCollection *collection, IDWriteFontFamily **family)
{
    struct dwrite_fontfamily *This;

    *family = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontfamily));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFamily_iface.lpVtbl = &fontfamilyvtbl;
    This->ref = 1;
    This->collection = collection;
    IDWriteFontCollection_AddRef(collection);
    This->data = data;
    InterlockedIncrement(&This->data->ref);

    *family = &This->IDWriteFontFamily_iface;

    return S_OK;
}

BOOL is_system_collection(IDWriteFontCollection *collection)
{
    void *obj;
    return IDWriteFontCollection_QueryInterface(collection, &IID_issystemcollection, (void**)&obj) == S_OK;
}

static HRESULT WINAPI dwritefontcollection_QueryInterface(IDWriteFontCollection *iface, REFIID riid, void **obj)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection))
    {
        *obj = iface;
        IDWriteFontCollection_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    if (This->is_system && IsEqualIID(riid, &IID_issystemcollection))
        return S_OK;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontcollection_AddRef(IDWriteFontCollection *iface)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontcollection_Release(IDWriteFontCollection *iface)
{
    unsigned int i;
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        for (i = 0; i < This->family_count; i++)
            release_fontfamily_data(This->family_data[i]);
        heap_free(This->family_data);
        heap_free(This);
    }

    return ref;
}

static UINT32 WINAPI dwritefontcollection_GetFontFamilyCount(IDWriteFontCollection *iface)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)\n", This);
    return This->family_count;
}

static HRESULT WINAPI dwritefontcollection_GetFontFamily(IDWriteFontCollection *iface, UINT32 index, IDWriteFontFamily **family)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);

    TRACE("(%p)->(%u %p)\n", This, index, family);

    if (index >= This->family_count) {
        *family = NULL;
        return E_FAIL;
    }

    return create_fontfamily(This->family_data[index], iface, family);
}

static UINT32 collection_find_family(struct dwrite_fontcollection *collection, const WCHAR *name)
{
    UINT32 i;

    for (i = 0; i < collection->family_count; i++) {
        IDWriteLocalizedStrings *family_name = collection->family_data[i]->familyname;
        HRESULT hr;
        int j;

        for (j = 0; j < IDWriteLocalizedStrings_GetCount(family_name); j++) {
            WCHAR buffer[255];
            hr = IDWriteLocalizedStrings_GetString(family_name, j, buffer, 255);
            if (SUCCEEDED(hr) && !strcmpW(buffer, name))
                return i;
        }
    }

    return ~0u;
}

static HRESULT WINAPI dwritefontcollection_FindFamilyName(IDWriteFontCollection *iface, const WCHAR *name, UINT32 *index, BOOL *exists)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(name), index, exists);
    *index = collection_find_family(This, name);
    *exists = *index != ~0u;
    return S_OK;
}

static BOOL is_same_fontfile(IDWriteFontFile *left, IDWriteFontFile *right)
{
    UINT32 left_key_size, right_key_size;
    const void *left_key, *right_key;
    HRESULT hr;

    if (left == right)
        return TRUE;

    hr = IDWriteFontFile_GetReferenceKey(left, &left_key, &left_key_size);
    if (FAILED(hr))
        return FALSE;

    hr = IDWriteFontFile_GetReferenceKey(right, &right_key, &right_key_size);
    if (FAILED(hr))
        return FALSE;

    if (left_key_size != right_key_size)
        return FALSE;

    return !memcmp(left_key, right_key, left_key_size);
}

static HRESULT WINAPI dwritefontcollection_GetFontFromFontFace(IDWriteFontCollection *iface, IDWriteFontFace *face, IDWriteFont **font)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    struct dwrite_fontfamily_data *found_family = NULL;
    struct dwrite_font_data *found_font = NULL;
    DWRITE_FONT_SIMULATIONS simulations;
    IDWriteFontFamily *family;
    UINT32 i, j, face_index;
    IDWriteFontFile *file;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, face, font);

    *font = NULL;

    if (!face)
        return E_INVALIDARG;

    i = 1;
    hr = IDWriteFontFace_GetFiles(face, &i, &file);
    if (FAILED(hr))
        return hr;
    face_index = IDWriteFontFace_GetIndex(face);

    for (i = 0; i < This->family_count; i++) {
        struct dwrite_fontfamily_data *family_data = This->family_data[i];
        for (j = 0; j < family_data->font_count; j++) {
            struct dwrite_font_data *font_data = family_data->fonts[j];

            if (face_index == font_data->face_index && is_same_fontfile(file, font_data->file)) {
                found_font = font_data;
                found_family = family_data;
                break;
            }
        }
    }

    if (!found_font)
        return E_INVALIDARG;

    hr = create_fontfamily(found_family, iface, &family);
    if (FAILED(hr))
        return hr;

    simulations = IDWriteFontFace_GetSimulations(face);
    hr = create_font(found_font, family, simulations, font);
    IDWriteFontFamily_Release(family);
    return hr;
}

static const IDWriteFontCollectionVtbl fontcollectionvtbl = {
    dwritefontcollection_QueryInterface,
    dwritefontcollection_AddRef,
    dwritefontcollection_Release,
    dwritefontcollection_GetFontFamilyCount,
    dwritefontcollection_GetFontFamily,
    dwritefontcollection_FindFamilyName,
    dwritefontcollection_GetFontFromFontFace
};

static HRESULT fontfamily_add_font(struct dwrite_fontfamily_data *family_data, struct dwrite_font_data *font_data)
{
    if (family_data->font_count + 1 >= family_data->font_alloc) {
        struct dwrite_font_data **new_list;
        UINT32 new_alloc;

        new_alloc = family_data->font_alloc * 2;
        new_list = heap_realloc(family_data->fonts, sizeof(*family_data->fonts) * new_alloc);
        if (!new_list)
            return E_OUTOFMEMORY;
        family_data->fonts = new_list;
        family_data->font_alloc = new_alloc;
    }

    family_data->fonts[family_data->font_count] = font_data;
    family_data->font_count++;
    return S_OK;
}

static HRESULT fontcollection_add_family(struct dwrite_fontcollection *collection, struct dwrite_fontfamily_data *family)
{
    if (collection->family_alloc < collection->family_count + 1) {
        struct dwrite_fontfamily_data **new_list;
        UINT32 new_alloc;

        new_alloc = collection->family_alloc * 2;
        new_list = heap_realloc(collection->family_data, sizeof(*new_list) * new_alloc);
        if (!new_list)
            return E_OUTOFMEMORY;

        collection->family_alloc = new_alloc;
        collection->family_data = new_list;
    }

    collection->family_data[collection->family_count] = family;
    collection->family_count++;

    return S_OK;
}

static HRESULT init_font_collection(struct dwrite_fontcollection *collection, BOOL is_system)
{
    collection->IDWriteFontCollection_iface.lpVtbl = &fontcollectionvtbl;
    collection->ref = 1;
    collection->family_count = 0;
    collection->family_alloc = 2;
    collection->is_system = is_system;

    collection->family_data = heap_alloc(sizeof(*collection->family_data)*2);
    if (!collection->family_data)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT get_filestream_from_file(IDWriteFontFile *file, IDWriteFontFileStream **stream)
{
    IDWriteFontFileLoader *loader;
    const void *key;
    UINT32 key_size;
    HRESULT hr;

    *stream = NULL;

    hr = IDWriteFontFile_GetReferenceKey(file, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(file, &loader);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFileLoader_CreateStreamFromKey(loader, key, key_size, stream);
    IDWriteFontFileLoader_Release(loader);
    if (FAILED(hr))
        return hr;

    return hr;
}

static HRESULT init_font_data(IDWriteFactory2 *factory, IDWriteFontFile *file, UINT32 face_index, DWRITE_FONT_FACE_TYPE face_type,
    IDWriteFontFileStream **stream, struct dwrite_font_data **ret)
{
    void *os2_context, *head_context;
    const void *tt_os2 = NULL, *tt_head = NULL;
    struct dwrite_font_data *data;
    HRESULT hr;

    data = heap_alloc_zero(sizeof(*data));
    if (!data)
        return E_OUTOFMEMORY;

    hr = get_filestream_from_file(file, stream);
    if (FAILED(hr)) {
        heap_free(data);
        return hr;
    }

    data->ref = 1;
    data->factory = factory;
    data->file = file;
    data->face_index = face_index;
    data->face_type = face_type;
    IDWriteFontFile_AddRef(file);
    IDWriteFactory2_AddRef(factory);

    opentype_get_font_table(*stream, face_type, face_index, MS_OS2_TAG, &tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(*stream, face_type, face_index, MS_HEAD_TAG, &tt_head, &head_context, NULL, NULL);

    opentype_get_font_properties(*stream, face_type, face_index, &data->stretch, &data->weight, &data->style);
    opentype_get_font_metrics(*stream, face_type, face_index, &data->metrics, NULL);

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(*stream, os2_context);
    if (tt_head)
        IDWriteFontFileStream_ReleaseFileFragment(*stream, head_context);

    *ret = data;
    return S_OK;
}

static HRESULT init_fontfamily_data(IDWriteLocalizedStrings *familyname, struct dwrite_fontfamily_data **ret)
{
    struct dwrite_fontfamily_data *data;

    data = heap_alloc(sizeof(*data));
    if (!data)
        return E_OUTOFMEMORY;

    data->ref = 1;
    data->font_count = 0;
    data->font_alloc = 2;

    data->fonts = heap_alloc(sizeof(*data->fonts)*data->font_alloc);
    if (!data->fonts) {
        heap_free(data);
        return E_OUTOFMEMORY;
    }

    data->familyname = familyname;
    IDWriteLocalizedStrings_AddRef(familyname);

    *ret = data;
    return S_OK;
}

HRESULT create_font_collection(IDWriteFactory2* factory, IDWriteFontFileEnumerator *enumerator, BOOL is_system, IDWriteFontCollection **ret)
{
    struct dwrite_fontcollection *collection;
    BOOL current = FALSE;
    HRESULT hr = S_OK;

    *ret = NULL;

    collection = heap_alloc(sizeof(struct dwrite_fontcollection));
    if (!collection) return E_OUTOFMEMORY;

    hr = init_font_collection(collection, is_system);
    if (FAILED(hr)) {
        heap_free(collection);
        return hr;
    }

    *ret = &collection->IDWriteFontCollection_iface;

    TRACE("building font collection:\n");

    while (hr == S_OK) {
        DWRITE_FONT_FACE_TYPE face_type;
        DWRITE_FONT_FILE_TYPE file_type;
        IDWriteFontFile *file;
        UINT32 face_count;
        BOOL supported;
        int i;

        current = FALSE;
        hr = IDWriteFontFileEnumerator_MoveNext(enumerator, &current);
        if (FAILED(hr) || !current)
            break;

        hr = IDWriteFontFileEnumerator_GetCurrentFontFile(enumerator, &file);
        if (FAILED(hr))
            break;

        hr = IDWriteFontFile_Analyze(file, &supported, &file_type, &face_type, &face_count);
        if (FAILED(hr) || !supported || face_count == 0) {
            TRACE("unsupported font (0x%08x, %d, %u)\n", hr, supported, face_count);
            IDWriteFontFile_Release(file);
            continue;
        }

        for (i = 0; i < face_count; i++) {
            IDWriteLocalizedStrings *family_name = NULL;
            struct dwrite_font_data *font_data;
            const void *name_table;
            void *name_context;
            IDWriteFontFileStream *stream;
            WCHAR buffer[255];
            UINT32 index;

            /* alloc and init new font data structure */
            hr = init_font_data(factory, file, i, face_type, &stream, &font_data);
            if (FAILED(hr))
                break;

            /* get family name from font file */
            name_table = NULL;
            opentype_get_font_table(stream, face_type, i, MS_NAME_TAG, &name_table, &name_context, NULL, NULL);
            if (name_table) {
                hr = opentype_get_font_strings_from_id(name_table, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &family_name);
                IDWriteFontFileStream_ReleaseFileFragment(stream, name_context);
            }
            IDWriteFontFileStream_Release(stream);

            if (FAILED(hr) || !family_name) {
                WARN("unable to get family name from font\n");
                release_font_data(font_data);
                continue;
            }

            buffer[0] = 0;
            IDWriteLocalizedStrings_GetString(family_name, 0, buffer, sizeof(buffer)/sizeof(WCHAR));

            index = collection_find_family(collection, buffer);
            if (index != ~0u)
                hr = fontfamily_add_font(collection->family_data[index], font_data);
            else {
                struct dwrite_fontfamily_data *family_data;

                /* create and init new family */
                hr = init_fontfamily_data(family_name, &family_data);
                if (hr == S_OK) {
                    /* add font to family, family - to collection */
                    hr = fontfamily_add_font(family_data, font_data);
                    if (hr == S_OK)
                        hr = fontcollection_add_family(collection, family_data);

                    if (FAILED(hr))
                        release_fontfamily_data(family_data);
                }
            }

            IDWriteLocalizedStrings_Release(family_name);

            if (FAILED(hr))
                break;
        }

        IDWriteFontFile_Release(file);
    };

    return hr;
}

struct system_fontfile_enumerator
{
    IDWriteFontFileEnumerator IDWriteFontFileEnumerator_iface;
    LONG ref;

    IDWriteFactory2 *factory;
    HKEY hkey;
    int index;
};

static inline struct system_fontfile_enumerator *impl_from_IDWriteFontFileEnumerator(IDWriteFontFileEnumerator* iface)
{
    return CONTAINING_RECORD(iface, struct system_fontfile_enumerator, IDWriteFontFileEnumerator_iface);
}

static HRESULT WINAPI systemfontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualIID(riid, &IID_IDWriteFontFileEnumerator) || IsEqualIID(riid, &IID_IUnknown)) {
        IDWriteFontFileEnumerator_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI systemfontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    return InterlockedIncrement(&enumerator->ref);
}

static ULONG WINAPI systemfontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    ULONG ref = InterlockedDecrement(&enumerator->ref);

    if (!ref) {
        IDWriteFactory2_Release(enumerator->factory);
        RegCloseKey(enumerator->hkey);
        heap_free(enumerator);
    }

    return ref;
}

static HRESULT WINAPI systemfontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface, IDWriteFontFile **file)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    DWORD ret, type, count;
    WCHAR *filename;
    HRESULT hr;

    *file = NULL;

    if (enumerator->index < 0)
        return E_FAIL;

    if (RegEnumValueW(enumerator->hkey, enumerator->index, NULL, NULL, NULL, &type, NULL, &count))
        return E_FAIL;

    if (!(filename = heap_alloc(count)))
        return E_OUTOFMEMORY;

    ret = RegEnumValueW(enumerator->hkey, enumerator->index, NULL, NULL, NULL, &type, (BYTE*)filename, &count);
    if (ret) {
        heap_free(filename);
        return E_FAIL;
    }

    /* Fonts installed in 'Fonts' system dir don't get full path in registry font files cache */
    if (!strchrW(filename, '\\')) {
        static const WCHAR fontsW[] = {'\\','f','o','n','t','s','\\',0};
        WCHAR fullpathW[MAX_PATH];

        GetWindowsDirectoryW(fullpathW, sizeof(fullpathW)/sizeof(WCHAR));
        strcatW(fullpathW, fontsW);
        strcatW(fullpathW, filename);

        hr = IDWriteFactory2_CreateFontFileReference(enumerator->factory, fullpathW, NULL, file);
    }
    else
        hr = IDWriteFactory2_CreateFontFileReference(enumerator->factory, filename, NULL, file);

    heap_free(filename);
    return hr;
}

static HRESULT WINAPI systemfontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);

    *current = FALSE;
    enumerator->index++;

    /* iterate until we find next string value */
    while (1) {
        DWORD type = 0, count;
        if (RegEnumValueW(enumerator->hkey, enumerator->index, NULL, NULL, NULL, &type, NULL, &count))
            break;
        if (type == REG_SZ) {
            *current = TRUE;
            break;
        }
        enumerator->index++;
    }

    TRACE("index = %d, current = %d\n", enumerator->index, *current);
    return S_OK;
}

static const struct IDWriteFontFileEnumeratorVtbl systemfontfileenumeratorvtbl =
{
    systemfontfileenumerator_QueryInterface,
    systemfontfileenumerator_AddRef,
    systemfontfileenumerator_Release,
    systemfontfileenumerator_MoveNext,
    systemfontfileenumerator_GetCurrentFontFile
};

static HRESULT create_system_fontfile_enumerator(IDWriteFactory2 *factory, IDWriteFontFileEnumerator **ret)
{
    struct system_fontfile_enumerator *enumerator;
    static const WCHAR fontslistW[] = {
        'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'F','o','n','t','s',0
    };

    *ret = NULL;

    enumerator = heap_alloc(sizeof(*enumerator));
    if (!enumerator)
        return E_OUTOFMEMORY;

    enumerator->IDWriteFontFileEnumerator_iface.lpVtbl = &systemfontfileenumeratorvtbl;
    enumerator->ref = 1;
    enumerator->factory = factory;
    enumerator->index = -1;
    IDWriteFactory2_AddRef(factory);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fontslistW, 0, GENERIC_READ, &enumerator->hkey)) {
        ERR("failed to open fonts list key\n");
        IDWriteFactory2_Release(factory);
        heap_free(enumerator);
        return E_FAIL;
    }

    *ret = &enumerator->IDWriteFontFileEnumerator_iface;

    return S_OK;
}

HRESULT get_system_fontcollection(IDWriteFactory2 *factory, IDWriteFontCollection **collection)
{
    IDWriteFontFileEnumerator *enumerator;
    HRESULT hr;

    *collection = NULL;

    hr = create_system_fontfile_enumerator(factory, &enumerator);
    if (FAILED(hr))
        return hr;

    TRACE("building system font collection for factory %p\n", factory);
    hr = create_font_collection(factory, enumerator, TRUE, collection);
    IDWriteFontFileEnumerator_Release(enumerator);
    return hr;
}

static HRESULT WINAPI eudcfontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualIID(riid, &IID_IDWriteFontFileEnumerator) || IsEqualIID(riid, &IID_IUnknown)) {
        IDWriteFontFileEnumerator_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI eudcfontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    return 2;
}

static ULONG WINAPI eudcfontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    return 1;
}

static HRESULT WINAPI eudcfontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface, IDWriteFontFile **file)
{
    *file = NULL;
    return E_FAIL;
}

static HRESULT WINAPI eudcfontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    *current = FALSE;
    return S_OK;
}

static const struct IDWriteFontFileEnumeratorVtbl eudcfontfileenumeratorvtbl =
{
    eudcfontfileenumerator_QueryInterface,
    eudcfontfileenumerator_AddRef,
    eudcfontfileenumerator_Release,
    eudcfontfileenumerator_MoveNext,
    eudcfontfileenumerator_GetCurrentFontFile
};

static IDWriteFontFileEnumerator eudc_fontfile_enumerator = { &eudcfontfileenumeratorvtbl };

HRESULT get_eudc_fontcollection(IDWriteFactory2 *factory, IDWriteFontCollection **collection)
{
    TRACE("building EUDC font collection for factory %p\n", factory);
    return create_font_collection(factory, &eudc_fontfile_enumerator, FALSE, collection);
}

static HRESULT WINAPI dwritefontfile_QueryInterface(IDWriteFontFile *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFile))
    {
        *obj = iface;
        IDWriteFontFile_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontfile_AddRef(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontfile_Release(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDWriteFontFileLoader_Release(This->loader);
        if (This->stream) IDWriteFontFileStream_Release(This->stream);
        heap_free(This->reference_key);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritefontfile_GetReferenceKey(IDWriteFontFile *iface, const void **fontFileReferenceKey, UINT32 *fontFileReferenceKeySize)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    TRACE("(%p)->(%p, %p)\n", This, fontFileReferenceKey, fontFileReferenceKeySize);
    *fontFileReferenceKey = This->reference_key;
    *fontFileReferenceKeySize = This->key_size;

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_GetLoader(IDWriteFontFile *iface, IDWriteFontFileLoader **fontFileLoader)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    TRACE("(%p)->(%p)\n", This, fontFileLoader);
    *fontFileLoader = This->loader;
    IDWriteFontFileLoader_AddRef(This->loader);

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_Analyze(IDWriteFontFile *iface, BOOL *isSupportedFontType, DWRITE_FONT_FILE_TYPE *fontFileType, DWRITE_FONT_FACE_TYPE *fontFaceType, UINT32 *numberOfFaces)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    IDWriteFontFileStream *stream;
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p, %p)\n", This, isSupportedFontType, fontFileType, fontFaceType, numberOfFaces);

    *isSupportedFontType = FALSE;
    *fontFileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    if (fontFaceType)
        *fontFaceType = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *numberOfFaces = 0;

    hr = IDWriteFontFileLoader_CreateStreamFromKey(This->loader, This->reference_key, This->key_size, &stream);
    if (FAILED(hr))
        return S_OK;

    hr = opentype_analyze_font(stream, numberOfFaces, fontFileType, fontFaceType, isSupportedFontType);

    /* TODO: Further Analysis */
    IDWriteFontFileStream_Release(stream);
    return S_OK;
}

static const IDWriteFontFileVtbl dwritefontfilevtbl = {
    dwritefontfile_QueryInterface,
    dwritefontfile_AddRef,
    dwritefontfile_Release,
    dwritefontfile_GetReferenceKey,
    dwritefontfile_GetLoader,
    dwritefontfile_Analyze,
};

HRESULT create_font_file(IDWriteFontFileLoader *loader, const void *reference_key, UINT32 key_size, IDWriteFontFile **font_file)
{
    struct dwrite_fontfile *This;

    This = heap_alloc(sizeof(struct dwrite_fontfile));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFile_iface.lpVtbl = &dwritefontfilevtbl;
    This->ref = 1;
    IDWriteFontFileLoader_AddRef(loader);
    This->loader = loader;
    This->stream = NULL;
    This->reference_key = heap_alloc(key_size);
    memcpy(This->reference_key, reference_key, key_size);
    This->key_size = key_size;

    *font_file = &This->IDWriteFontFile_iface;

    return S_OK;
}

static HRESULT get_stream_from_file(IDWriteFontFile *file, IDWriteFontFileStream **stream)
{
    IDWriteFontFileLoader *loader;
    UINT32 key_size;
    const void *key;
    HRESULT hr;

    *stream = NULL;
    hr = IDWriteFontFile_GetLoader(file, &loader);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetReferenceKey(file, &key, &key_size);
    if (FAILED(hr)) {
        IDWriteFontFileLoader_Release(loader);
        return hr;
    }

    hr = IDWriteFontFileLoader_CreateStreamFromKey(loader, key, key_size, stream);
    IDWriteFontFileLoader_Release(loader);

    return hr;
}

HRESULT create_fontface(DWRITE_FONT_FACE_TYPE facetype, UINT32 files_number, IDWriteFontFile* const* font_files, UINT32 index,
    DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace2 **ret)
{
    struct dwrite_fontface *fontface;
    HRESULT hr = S_OK;
    int i;

    *ret = NULL;

    fontface = heap_alloc(sizeof(struct dwrite_fontface));
    if (!fontface)
        return E_OUTOFMEMORY;

    fontface->files = heap_alloc_zero(sizeof(*fontface->files) * files_number);
    fontface->streams = heap_alloc_zero(sizeof(*fontface->streams) * files_number);

    if (!fontface->files || !fontface->streams) {
        heap_free(fontface->files);
        heap_free(fontface->streams);
        heap_free(fontface);
        return E_OUTOFMEMORY;
    }

    fontface->IDWriteFontFace2_iface.lpVtbl = &dwritefontfacevtbl;
    fontface->ref = 1;
    fontface->type = facetype;
    fontface->file_count = files_number;
    fontface->cmap.data = NULL;
    fontface->cmap.context = NULL;
    fontface->cmap.size = 0;
    fontface->index = index;
    fontface->simulations = simulations;
    memset(fontface->glyphs, 0, sizeof(fontface->glyphs));

    for (i = 0; i < fontface->file_count; i++) {
        hr = get_stream_from_file(font_files[i], &fontface->streams[i]);
        if (FAILED(hr)) {
            IDWriteFontFace2_Release(&fontface->IDWriteFontFace2_iface);
            return hr;
        }

        fontface->files[i] = font_files[i];
        IDWriteFontFile_AddRef(font_files[i]);
    }

    opentype_get_font_metrics(fontface->streams[0], facetype, index, &fontface->metrics, &fontface->caret);
    if (simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE) {
        /* TODO: test what happens if caret is already slanted */
        if (fontface->caret.slopeRise == 1) {
            fontface->caret.slopeRise = fontface->metrics.designUnitsPerEm;
            fontface->caret.slopeRun = fontface->caret.slopeRise / 3;
        }
    }

    *ret = &fontface->IDWriteFontFace2_iface;
    return S_OK;
}

/* IDWriteLocalFontFileLoader and its required IDWriteFontFileStream */
struct local_refkey
{
    FILETIME writetime;
    WCHAR name[1];
};

struct local_cached_stream
{
    struct list entry;
    IDWriteFontFileStream *stream;
    struct local_refkey *key;
    UINT32 key_size;
};

struct dwrite_localfontfilestream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG ref;

    struct local_cached_stream *entry;
    const void *file_ptr;
    UINT64 size;
};

struct dwrite_localfontfileloader {
    IDWriteLocalFontFileLoader IDWriteLocalFontFileLoader_iface;
    LONG ref;

    struct list streams;
};

static inline struct dwrite_localfontfileloader *impl_from_IDWriteLocalFontFileLoader(IDWriteLocalFontFileLoader *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_localfontfileloader, IDWriteLocalFontFileLoader_iface);
}

static inline struct dwrite_localfontfilestream *impl_from_IDWriteFontFileStream(IDWriteFontFileStream *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_localfontfilestream, IDWriteFontFileStream_iface);
}

static HRESULT WINAPI localfontfilestream_QueryInterface(IDWriteFontFileStream *iface, REFIID riid, void **obj)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream))
    {
        *obj = iface;
        IDWriteFontFileStream_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI localfontfilestream_AddRef(IDWriteFontFileStream *iface)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static inline void release_cached_stream(struct local_cached_stream *stream)
{
    list_remove(&stream->entry);
    heap_free(stream->key);
    heap_free(stream);
}

static ULONG WINAPI localfontfilestream_Release(IDWriteFontFileStream *iface)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        UnmapViewOfFile(This->file_ptr);
        release_cached_stream(This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI localfontfilestream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start, UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);

    TRACE("(%p)->(%p, %s, %s, %p)\n",This, fragment_start,
          wine_dbgstr_longlong(offset), wine_dbgstr_longlong(fragment_size), fragment_context);

    *fragment_context = NULL;

    if ((offset >= This->size - 1) || (fragment_size > This->size - offset)) {
        *fragment_start = NULL;
        return E_FAIL;
    }

    *fragment_start = (char*)This->file_ptr + offset;
    return S_OK;
}

static void WINAPI localfontfilestream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    TRACE("(%p)->(%p)\n", This, fragment_context);
}

static HRESULT WINAPI localfontfilestream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    TRACE("(%p)->(%p)\n", This, size);
    *size = This->size;
    return S_OK;
}

static HRESULT WINAPI localfontfilestream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    ULARGE_INTEGER li;

    TRACE("(%p)->(%p)\n", This, last_writetime);

    li.u.LowPart = This->entry->key->writetime.dwLowDateTime;
    li.u.HighPart = This->entry->key->writetime.dwHighDateTime;
    *last_writetime = li.QuadPart;

    return S_OK;
}

static const IDWriteFontFileStreamVtbl localfontfilestreamvtbl =
{
    localfontfilestream_QueryInterface,
    localfontfilestream_AddRef,
    localfontfilestream_Release,
    localfontfilestream_ReadFileFragment,
    localfontfilestream_ReleaseFileFragment,
    localfontfilestream_GetFileSize,
    localfontfilestream_GetLastWriteTime
};

static HRESULT create_localfontfilestream(const void *file_ptr, UINT64 size, struct local_cached_stream *entry, IDWriteFontFileStream** iface)
{
    struct dwrite_localfontfilestream *This = heap_alloc(sizeof(struct dwrite_localfontfilestream));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDWriteFontFileStream_iface.lpVtbl = &localfontfilestreamvtbl;
    This->ref = 1;

    This->file_ptr = file_ptr;
    This->size = size;
    This->entry = entry;

    *iface = &This->IDWriteFontFileStream_iface;
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_QueryInterface(IDWriteLocalFontFileLoader *iface, REFIID riid, void **obj)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader) || IsEqualIID(riid, &IID_IDWriteLocalFontFileLoader))
    {
        *obj = iface;
        IDWriteLocalFontFileLoader_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI localfontfileloader_AddRef(IDWriteLocalFontFileLoader *iface)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI localfontfileloader_Release(IDWriteLocalFontFileLoader *iface)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        struct local_cached_stream *stream, *stream2;

        /* This will detach all entries from cache. Entries are released together with streams,
           so stream controls its lifetime. */
        LIST_FOR_EACH_ENTRY_SAFE(stream, stream2, &This->streams, struct local_cached_stream, entry)
            list_init(&stream->entry);

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI localfontfileloader_CreateStreamFromKey(IDWriteLocalFontFileLoader *iface, const void *key, UINT32 key_size, IDWriteFontFileStream **ret)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    const struct local_refkey *refkey = key;
    struct local_cached_stream *stream;
    IDWriteFontFileStream *filestream;
    HANDLE file, mapping;
    LARGE_INTEGER size;
    void *file_ptr;
    HRESULT hr;

    TRACE("(%p)->(%p, %i, %p)\n", This, key, key_size, ret);
    TRACE("name: %s\n", debugstr_w(refkey->name));

    /* search cache first */
    LIST_FOR_EACH_ENTRY(stream, &This->streams, struct local_cached_stream, entry) {
        if (key_size == stream->key_size && !memcmp(stream->key, key, key_size)) {
            *ret = stream->stream;
            IDWriteFontFileStream_AddRef(*ret);
            return S_OK;
        }
    }

    *ret = NULL;

    file = CreateFileW(refkey->name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return E_FAIL;

    GetFileSizeEx(file, &size);
    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if (!mapping)
        return E_FAIL;

    file_ptr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);

    stream = heap_alloc(sizeof(*stream));
    if (!stream) {
        UnmapViewOfFile(file_ptr);
        return E_OUTOFMEMORY;
    }

    stream->key = heap_alloc(key_size);
    if (!stream->key) {
        UnmapViewOfFile(file_ptr);
        heap_free(stream);
        return E_OUTOFMEMORY;
    }

    stream->key_size = key_size;
    memcpy(stream->key, key, key_size);

    hr = create_localfontfilestream(file_ptr, size.QuadPart, stream, &filestream);
    if (FAILED(hr)) {
        UnmapViewOfFile(file_ptr);
        heap_free(stream->key);
        heap_free(stream);
        return hr;
    }

    stream->stream = filestream;
    list_add_head(&This->streams, &stream->entry);

    *ret = stream->stream;

    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetFilePathLengthFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, UINT32 *length)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    const struct local_refkey *refkey = key;

    TRACE("(%p)->(%p, %i, %p)\n", This, key, key_size, length);

    *length = strlenW(refkey->name);
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetFilePathFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, WCHAR *path, UINT32 length)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    const struct local_refkey *refkey = key;

    TRACE("(%p)->(%p, %i, %p, %i)\n", This, key, key_size, path, length);

    if (length < strlenW(refkey->name))
        return E_INVALIDARG;

    strcpyW(path, refkey->name);
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetLastWriteTimeFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, FILETIME *writetime)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    const struct local_refkey *refkey = key;

    TRACE("(%p)->(%p, %i, %p)\n", This, key, key_size, writetime);

    *writetime = refkey->writetime;
    return S_OK;
}

static const struct IDWriteLocalFontFileLoaderVtbl localfontfileloadervtbl = {
    localfontfileloader_QueryInterface,
    localfontfileloader_AddRef,
    localfontfileloader_Release,
    localfontfileloader_CreateStreamFromKey,
    localfontfileloader_GetFilePathLengthFromKey,
    localfontfileloader_GetFilePathFromKey,
    localfontfileloader_GetLastWriteTimeFromKey
};

HRESULT create_localfontfileloader(IDWriteLocalFontFileLoader** iface)
{
    struct dwrite_localfontfileloader *This = heap_alloc(sizeof(struct dwrite_localfontfileloader));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDWriteLocalFontFileLoader_iface.lpVtbl = &localfontfileloadervtbl;
    This->ref = 1;
    list_init(&This->streams);

    *iface = &This->IDWriteLocalFontFileLoader_iface;
    return S_OK;
}

HRESULT get_local_refkey(const WCHAR *path, const FILETIME *writetime, void **key, UINT32 *size)
{
    struct local_refkey *refkey;

    *size = FIELD_OFFSET(struct local_refkey, name) + (strlenW(path)+1)*sizeof(WCHAR);
    *key = NULL;

    refkey = heap_alloc(*size);
    if (!refkey)
        return E_OUTOFMEMORY;

    if (writetime)
        refkey->writetime = *writetime;
    else {
        WIN32_FILE_ATTRIBUTE_DATA info;

        if (GetFileAttributesExW(path, GetFileExInfoStandard, &info))
            refkey->writetime = info.ftLastWriteTime;
        else
            memset(&refkey->writetime, 0, sizeof(refkey->writetime));
    }
    strcpyW(refkey->name, path);

    *key = refkey;

    return S_OK;
}
