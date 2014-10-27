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

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define MS_HEAD_TAG DWRITE_MAKE_OPENTYPE_TAG('h','e','a','d')
#define MS_OS2_TAG  DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2')
#define MS_POST_TAG DWRITE_MAKE_OPENTYPE_TAG('p','o','s','t')
#define MS_CMAP_TAG DWRITE_MAKE_OPENTYPE_TAG('c','m','a','p')
#define MS_NAME_TAG DWRITE_MAKE_OPENTYPE_TAG('n','a','m','e')

struct dwrite_font_data {
    LONG ref;

    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_SIMULATIONS simulations;
    DWRITE_FONT_METRICS metrics;
    IDWriteLocalizedStrings *info_strings[DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME+1];

    /* data needed to create fontface instance */
    IDWriteFactory *factory;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFile *file;
    UINT32 face_index;

    IDWriteFontFace2 *face;

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

    WCHAR **families;
    UINT32 count;
    int alloc;

    struct dwrite_fontfamily_data **family_data;
    UINT32 family_count;
    UINT32 family_alloc;
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

    BOOL is_system;
    IDWriteFontFamily *family;

    struct dwrite_font_data *data;
};

#define DWRITE_FONTTABLE_MAGIC 0xededfafa

struct dwrite_fonttablecontext {
    UINT32 magic;
    void  *context;
    UINT32 file_index;
};

struct dwrite_fonttable {
    void  *data;
    void  *context;
    UINT32 size;
};

struct dwrite_fontface {
    IDWriteFontFace2 IDWriteFontFace2_iface;
    LONG ref;

    IDWriteFontFile **files;
    UINT32 file_count;
    UINT32 index;

    DWRITE_FONT_SIMULATIONS simulations;
    DWRITE_FONT_FACE_TYPE type;

    struct dwrite_fonttable cmap;

    BOOL is_system;
    LOGFONTW logfont;
};

struct dwrite_fontfile {
    IDWriteFontFile IDWriteFontFile_iface;
    LONG ref;

    IDWriteFontFileLoader *loader;
    void *reference_key;
    UINT32 key_size;
    IDWriteFontFileStream *stream;
};

static HRESULT create_fontfamily(IDWriteLocalizedStrings *familyname, IDWriteFontFamily **family);
static HRESULT create_fontfamily_from_data(struct dwrite_fontfamily_data *data, IDWriteFontCollection *collection, IDWriteFontFamily **family);
static HRESULT create_font_base(IDWriteFont **font);
static HRESULT create_font_from_data(struct dwrite_font_data*,IDWriteFontFamily*,IDWriteFont**);

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

static HRESULT _dwritefontfile_GetFontFileStream(IDWriteFontFile *iface, IDWriteFontFileStream **stream)
{
    HRESULT hr;
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    if (!This->stream)
    {
        hr = IDWriteFontFileLoader_CreateStreamFromKey(This->loader, This->reference_key, This->key_size, &This->stream);
        if (FAILED(hr))
            return hr;
    }
    if (This->stream)
    {
        IDWriteFontFileStream_AddRef(This->stream);
        *stream = This->stream;
        return S_OK;
    }
    return E_FAIL;
}

static void release_font_data(struct dwrite_font_data *data)
{
    int i;
    if (!data)
        return;
    i = InterlockedDecrement(&data->ref);
    if (i > 0)
        return;

    for (i = DWRITE_INFORMATIONAL_STRING_NONE; i < sizeof(data->info_strings)/sizeof(data->info_strings[0]); i++) {
        if (data->info_strings[i])
            IDWriteLocalizedStrings_Release(data->info_strings[i]);
    }

    /* FIXME: factory and file will be always set once system collection is working */
    if (data->file)
        IDWriteFontFile_Release(data->file);
    if (data->factory)
        IDWriteFactory_Release(data->factory);
    if (data->face)
        IDWriteFontFace2_Release(data->face);
    heap_free(data->facename);
    heap_free(data);
}

static VOID _free_fontfamily_data(struct dwrite_fontfamily_data *data)
{
    int i;
    if (!data)
        return;
    i = InterlockedDecrement(&data->ref);
    if (i > 0)
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
        for (i = 0; i < This->file_count; i++)
            IDWriteFontFile_Release(This->files[i]);
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
    FIXME("(%p)->(%p): stub\n", This, metrics);
}

static UINT16 WINAPI dwritefontface_GetGlyphCount(IDWriteFontFace2 *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static HRESULT WINAPI dwritefontface_GetDesignGlyphMetrics(IDWriteFontFace2 *iface,
    UINT16 const *glyph_indices, UINT32 glyph_count, DWRITE_GLYPH_METRICS *metrics, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%p %u %p %d): stub\n", This, glyph_indices, glyph_count, metrics, is_sideways);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetGlyphIndices(IDWriteFontFace2 *iface, UINT32 const *codepoints,
    UINT32 count, UINT16 *glyph_indices)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    unsigned int i;

    if (This->is_system)
    {
        HFONT hfont;
        WCHAR *str;
        HDC hdc;

        TRACE("(%p)->(%p %u %p)\n", This, codepoints, count, glyph_indices);

        str = heap_alloc(count*sizeof(WCHAR));
        if (!str) return E_OUTOFMEMORY;

        for (i = 0; i < count; i++)
            str[i] = codepoints[i] < 0x10000 ? codepoints[i] : '?';

        hdc = CreateCompatibleDC(0);
        hfont = CreateFontIndirectW(&This->logfont);
        SelectObject(hdc, hfont);

        GetGlyphIndicesW(hdc, str, count, glyph_indices, 0);
        heap_free(str);

        DeleteDC(hdc);
        DeleteObject(hfont);

        return S_OK;
    }
    else
    {
        void *data;

        TRACE("(%p)->(%p %u %p)\n", This, codepoints, count, glyph_indices);

        data = get_fontface_cmap(This);
        if (!data)
            return E_FAIL;

        for (i = 0; i < count; i++)
            opentype_cmap_get_glyphindex(data, codepoints[i], &glyph_indices[i]);

        return S_OK;
    }
}

static HRESULT WINAPI dwritefontface_TryGetFontTable(IDWriteFontFace2 *iface, UINT32 table_tag,
    const void **table_data, UINT32 *table_size, void **context, BOOL *exists)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    if (This->is_system)
    {
        FIXME("(%p)->(%u %p %p %p %p): stub\n", This, table_tag, table_data, table_size, context, exists);
        return E_NOTIMPL;
    }
    else
    {
        HRESULT hr = S_OK;
        int i;
        struct dwrite_fonttablecontext *tablecontext;

        TRACE("(%p)->(%u %p %p %p %p)\n", This, table_tag, table_data, table_size, context, exists);

        tablecontext = heap_alloc(sizeof(struct dwrite_fonttablecontext));
        if (!tablecontext)
            return E_OUTOFMEMORY;
        tablecontext->magic = DWRITE_FONTTABLE_MAGIC;

        *exists = FALSE;
        for (i = 0; i < This->file_count && !(*exists); i++)
        {
            IDWriteFontFileStream *stream;
            hr = _dwritefontfile_GetFontFileStream(This->files[i], &stream);
            if (FAILED(hr))
                continue;
            tablecontext->file_index = i;

            hr = opentype_get_font_table(stream, This->type, This->index, table_tag, table_data, &tablecontext->context, table_size, exists);

            IDWriteFontFileStream_Release(stream);
        }
        if (FAILED(hr) && !*exists)
            heap_free(tablecontext);
        else
            *context = (void*)tablecontext;
        return hr;
    }
}

static void WINAPI dwritefontface_ReleaseFontTable(IDWriteFontFace2 *iface, void *table_context)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    struct dwrite_fonttablecontext *tablecontext = (struct dwrite_fonttablecontext*)table_context;
    IDWriteFontFileStream *stream;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, table_context);

    if (tablecontext->magic != DWRITE_FONTTABLE_MAGIC)
    {
        TRACE("Invalid table magic\n");
        return;
    }

    hr = _dwritefontfile_GetFontFileStream(This->files[tablecontext->file_index], &stream);
    if (FAILED(hr))
        return;
    IDWriteFontFileStream_ReleaseFileFragment(stream, tablecontext->context);
    IDWriteFontFileStream_Release(stream);
    heap_free(tablecontext);
}

static HRESULT WINAPI dwritefontface_GetGlyphRunOutline(IDWriteFontFace2 *iface, FLOAT emSize,
    UINT16 const *glyph_indices, FLOAT const* glyph_advances, DWRITE_GLYPH_OFFSET const *glyph_offsets,
    UINT32 glyph_count, BOOL is_sideways, BOOL is_rtl, IDWriteGeometrySink *geometrysink)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%f %p %p %p %u %d %d %p): stub\n", This, emSize, glyph_indices, glyph_advances, glyph_offsets,
        glyph_count, is_sideways, is_rtl, geometrysink);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%f %f %p %p): stub\n", This, emSize, pixels_per_dip, transform, metrics);
    return E_NOTIMPL;
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

static HRESULT WINAPI dwritefontface1_GetMetrics(IDWriteFontFace2 *iface, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p): stub\n", This, metrics);
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
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritefontface1_GetDesignGlyphAdvances(IDWriteFontFace2 *iface,
    UINT32 glyph_count, UINT16 const *indices, INT32 *advances, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace2(iface);
    FIXME("(%p)->(%u %p %p %d): stub\n", This, glyph_count, indices, advances, is_sideways);
    return E_NOTIMPL;
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

static HRESULT create_system_fontface(struct dwrite_font *font, IDWriteFontFace **face)
{
    struct dwrite_fontface *This;

    *face = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontface));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFace2_iface.lpVtbl = &dwritefontfacevtbl;
    This->ref = 1;
    This->type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    This->file_count = 0;
    This->files = NULL;
    This->index = 0;
    This->simulations = DWRITE_FONT_SIMULATIONS_NONE;
    This->cmap.data = NULL;
    This->cmap.context = NULL;
    This->cmap.size = 0;

    This->is_system = TRUE;
    memset(&This->logfont, 0, sizeof(This->logfont));
    This->logfont.lfItalic = font->data->style == DWRITE_FONT_STYLE_ITALIC;
    /* weight values from DWRITE_FONT_WEIGHT match values used for LOGFONT */
    This->logfont.lfWeight = font->data->weight;
    strcpyW(This->logfont.lfFaceName, font->data->facename);

    *face = (IDWriteFontFace*)&This->IDWriteFontFace2_iface;

    return S_OK;
}

HRESULT convert_fontface_to_logfont(IDWriteFontFace *face, LOGFONTW *logfont)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace2((IDWriteFontFace2*)face);

    *logfont = fontface->logfont;

    return S_OK;
}

static HRESULT get_fontface_from_font(struct dwrite_font *font, IDWriteFontFace2 **fontface)
{
    HRESULT hr = S_OK;

    *fontface = NULL;

    if (!font->data->face) {
        struct dwrite_font_data *data = font->data;
        IDWriteFontFace *face;

        hr = font->is_system ? create_system_fontface(font, &face) :
            IDWriteFactory_CreateFontFace(data->factory, data->face_type, 1, &data->file,
                data->face_index, DWRITE_FONT_SIMULATIONS_NONE, &face);
        if (FAILED(hr))
            return hr;

        hr = IDWriteFontFace_QueryInterface(face, &IID_IDWriteFontFace2, (void**)&font->data->face);
        IDWriteFontFace_Release(face);
    }

    *fontface = font->data->face;
    return hr;
}

static HRESULT create_font_base(IDWriteFont **font)
{
    struct dwrite_font_data *data;
    HRESULT ret;

    *font = NULL;
    data = heap_alloc_zero(sizeof(*data));
    if (!data) return E_OUTOFMEMORY;

    ret = create_font_from_data( data, NULL, font );
    if (FAILED(ret)) heap_free( data );
    return ret;
}

static HRESULT create_font_from_logfont(const LOGFONTW *logfont, IDWriteFont **font)
{
    const WCHAR* facename, *familyname;
    IDWriteLocalizedStrings *name;
    struct dwrite_font *This;
    IDWriteFontFamily *family;
    OUTLINETEXTMETRICW *otm;
    HRESULT hr;
    HFONT hfont;
    HDC hdc;
    int ret;
    static const WCHAR enusW[] = {'e','n','-','u','s',0};
    LPVOID tt_os2 = NULL;
    LPVOID tt_head = NULL;
    LPVOID tt_post = NULL;
    LONG size;

    hr = create_font_base(font);
    if (FAILED(hr))
        return hr;

    This = impl_from_IDWriteFont2((IDWriteFont2*)*font);

    hfont = CreateFontIndirectW(logfont);
    if (!hfont)
    {
        heap_free(This->data);
        heap_free(This);
        return DWRITE_E_NOFONT;
    }

    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    ret = GetOutlineTextMetricsW(hdc, 0, NULL);
    otm = heap_alloc(ret);
    if (!otm)
    {
        heap_free(This->data);
        heap_free(This);
        DeleteDC(hdc);
        DeleteObject(hfont);
        return E_OUTOFMEMORY;
    }
    otm->otmSize = ret;
    ret = GetOutlineTextMetricsW(hdc, otm->otmSize, otm);

    size = GetFontData(hdc, MS_OS2_TAG, 0, NULL, 0);
    if (size != GDI_ERROR)
    {
        tt_os2 = heap_alloc(size);
        GetFontData(hdc, MS_OS2_TAG, 0, tt_os2, size);
    }
    size = GetFontData(hdc, MS_HEAD_TAG, 0, NULL, 0);
    if (size != GDI_ERROR)
    {
        tt_head = heap_alloc(size);
        GetFontData(hdc, MS_HEAD_TAG, 0, tt_head, size);
    }
    size = GetFontData(hdc, MS_POST_TAG, 0, NULL, 0);
    if (size != GDI_ERROR)
    {
        tt_post = heap_alloc(size);
        GetFontData(hdc, MS_POST_TAG, 0, tt_post, size);
    }

    get_font_properties(tt_os2, tt_head, tt_post, &This->data->metrics, &This->data->stretch, &This->data->weight, &This->data->style);
    heap_free(tt_os2);
    heap_free(tt_head);
    heap_free(tt_post);

    if (logfont->lfItalic)
        This->data->style = DWRITE_FONT_STYLE_ITALIC;

    DeleteDC(hdc);
    DeleteObject(hfont);

    facename = (WCHAR*)((char*)otm + (ptrdiff_t)otm->otmpFaceName);
    familyname = (WCHAR*)((char*)otm + (ptrdiff_t)otm->otmpFamilyName);
    TRACE("facename=%s, familyname=%s\n", debugstr_w(facename), debugstr_w(familyname));

    hr = create_localizedstrings(&name);
    if (FAILED(hr))
    {
        heap_free(This);
        return hr;
    }
    add_localizedstring(name, enusW, familyname);
    hr = create_fontfamily(name, &family);

    heap_free(otm);
    if (hr != S_OK)
    {
        heap_free(This->data);
        heap_free(This);
        return hr;
    }

    This->is_system = TRUE;
    This->family = family;
    This->data->simulations = DWRITE_FONT_SIMULATIONS_NONE;
    This->data->facename = heap_strdupW(logfont->lfFaceName);

    return S_OK;
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
        if (This->family) IDWriteFontFamily_Release(This->family);
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
    return This->data->style;
}

static BOOL WINAPI dwritefont_IsSymbolFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritefont_GetFaceNames(IDWriteFont2 *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    FIXME("(%p)->(%p): stub\n", This, names);
    return E_NOTIMPL;
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
    return This->data->simulations;
}

static void WINAPI dwritefont_GetMetrics(IDWriteFont2 *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);

    TRACE("(%p)->(%p)\n", This, metrics);
    *metrics = This->data->metrics;
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
    FIXME("(%p)->(%p): stub\n", This, metrics);
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

static HRESULT WINAPI dwritefont1_IsMonospacedFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritefont2_IsColorFont(IDWriteFont2 *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
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
        if (This->collection)
            IDWriteFontCollection_Release(This->collection);
        _free_fontfamily_data(This->data);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritefontfamily_GetFontCollection(IDWriteFontFamily *iface, IDWriteFontCollection **collection)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    TRACE("(%p)->(%p)\n", This, collection);
    if (This->collection)
    {
        IDWriteFontCollection_AddRef(This->collection);
        *collection = This->collection;
        return S_OK;
    }
    else
        return E_NOTIMPL;
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
    if (This->data->font_count > 0)
    {
        if (index >= This->data->font_count)
            return E_INVALIDARG;
        return create_font_from_data(This->data->fonts[index], iface, font);
    }
    else
        return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontfamily_GetFamilyNames(IDWriteFontFamily *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    return clone_localizedstring(This->data->familyname, names);
}

static HRESULT WINAPI dwritefontfamily_GetFirstMatchingFont(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    LOGFONTW lf;

    TRACE("(%p)->(%d %d %d %p)\n", This, weight, stretch, style, font);

    /* fallback for system font collections */
    if (This->data->font_count == 0)
    {
        memset(&lf, 0, sizeof(lf));
        lf.lfWeight = weight;
        lf.lfItalic = style == DWRITE_FONT_STYLE_ITALIC;
        IDWriteLocalizedStrings_GetString(This->data->familyname, 0, lf.lfFaceName, LF_FACESIZE);

        return create_font_from_logfont(&lf, font);
    }
    else
    {
        int i;
        for (i = 0; i < This->data->font_count; i++)
        {
            if (style == This->data->fonts[i]->style &&
                weight == This->data->fonts[i]->weight &&
                stretch == This->data->fonts[i]->stretch)
            {
                return create_font_from_data(This->data->fonts[i], iface, font);
            }
        }
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

    if (!ref)
    {
        for (i = 0; i < This->count; i++)
            heap_free(This->families[i]);
        heap_free(This->families);
        for (i = 0; i < This->family_count; i++)
            _free_fontfamily_data(This->family_data[i]);
        heap_free(This->family_data);
        heap_free(This);
    }

    return ref;
}

static UINT32 WINAPI dwritefontcollection_GetFontFamilyCount(IDWriteFontCollection *iface)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)\n", This);
    if (This->family_count)
        return This->family_count;
    return This->count;
}

static HRESULT WINAPI dwritefontcollection_GetFontFamily(IDWriteFontCollection *iface, UINT32 index, IDWriteFontFamily **family)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    HRESULT hr;
    IDWriteLocalizedStrings *familyname;
    static const WCHAR enusW[] = {'e','n','-','u','s',0};

    TRACE("(%p)->(%u %p)\n", This, index, family);

    if (This->family_count)
    {
        if (index >= This->family_count)
        {
            *family = NULL;
            return E_FAIL;
        }
        else
            return create_fontfamily_from_data(This->family_data[index], iface, family);
    }
    else
    {
        if (index >= This->count)
        {
            *family = NULL;
            return E_FAIL;
        }

        hr = create_localizedstrings(&familyname);
        if (FAILED(hr))
            return hr;
        add_localizedstring(familyname, enusW, This->families[index]);

        return create_fontfamily(familyname, family);
    }
}

static HRESULT collection_find_family(struct dwrite_fontcollection *collection, const WCHAR *name, UINT32 *index, BOOL *exists)
{
    UINT32 i;

    if (collection->family_count) {
        for (i = 0; i < collection->family_count; i++) {
            IDWriteLocalizedStrings *family_name = collection->family_data[i]->familyname;
            HRESULT hr;
            int j;

            for (j = 0; j < IDWriteLocalizedStrings_GetCount(family_name); j++) {
                WCHAR buffer[255];
                hr = IDWriteLocalizedStrings_GetString(family_name, j, buffer, 255);
                if (SUCCEEDED(hr)) {
                    if (!strcmpW(buffer, name)) {
                        *index = i;
                        *exists = TRUE;
                        return S_OK;
                    }
                }
            }
        }
        *index = (UINT32)-1;
        *exists = FALSE;
    }
    else {
        for (i = 0; i < collection->count; i++)
            if (!strcmpW(collection->families[i], name)) {
                *index = i;
                *exists = TRUE;
                return S_OK;
            }

        *index = (UINT32)-1;
        *exists = FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontcollection_FindFamilyName(IDWriteFontCollection *iface, const WCHAR *name, UINT32 *index, BOOL *exists)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(name), index, exists);
    return collection_find_family(This, name, index, exists);
}

static HRESULT WINAPI dwritefontcollection_GetFontFromFontFace(IDWriteFontCollection *iface, IDWriteFontFace *face, IDWriteFont **font)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    struct dwrite_fontfamily_data *found_family = NULL;
    struct dwrite_font_data *found_font = NULL;
    IDWriteFontFamily *family;
    UINT32 i, j;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, face, font);

    *font = NULL;

    if (!face)
        return E_INVALIDARG;

    for (i = 0; i < This->family_count; i++) {
        struct dwrite_fontfamily_data *family_data = This->family_data[i];
        for (j = 0; j < family_data->font_count; j++) {
            if ((IDWriteFontFace*)family_data->fonts[j]->face == face) {
                found_font = family_data->fonts[j];
                found_family = family_data;
                break;
            }
        }
    }

    if (!found_font)
        return E_INVALIDARG;

    hr = create_fontfamily_from_data(found_family, iface, &family);
    if (FAILED(hr))
        return hr;

    hr = create_font_from_data(found_font, family, font);
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

static HRESULT add_family_syscollection(struct dwrite_fontcollection *collection, const WCHAR *family)
{
    /* check for duplicate family name */
    if (collection->count && !strcmpW(collection->families[collection->count-1], family)) return S_OK;

    /* double array length */
    if (collection->count == collection->alloc)
    {
        collection->alloc *= 2;
        collection->families = heap_realloc(collection->families, collection->alloc*sizeof(WCHAR*));
    }

    collection->families[collection->count++] = heap_strdupW(family);
    TRACE("family name %s\n", debugstr_w(family));

    return S_OK;
}

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
    InterlockedIncrement(&font_data->ref);
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

static HRESULT init_font_collection(struct dwrite_fontcollection *collection)
{
    collection->IDWriteFontCollection_iface.lpVtbl = &fontcollectionvtbl;
    collection->ref = 1;
    collection->family_count = 0;
    collection->family_alloc = 2;
    collection->count = 0;
    collection->alloc = 0;
    collection->families = NULL;

    collection->family_data = heap_alloc(sizeof(*collection->family_data)*2);
    if (!collection->family_data)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT get_filestream_from_file(IDWriteFontFile *file, IDWriteFontFileStream **stream)
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

static HRESULT init_font_data(IDWriteFactory *factory, IDWriteFontFile *file, UINT32 face_index, DWRITE_FONT_FACE_TYPE face_type, struct dwrite_font_data *data)
{
    void *os2_context, *head_context, *post_context;
    const void *tt_os2 = NULL, *tt_head = NULL, *tt_post = NULL;
    IDWriteFontFileStream *stream;
    HRESULT hr;

    hr = get_filestream_from_file(file, &stream);
    if (FAILED(hr))
        return hr;

    data->factory = factory;
    data->file = file;
    data->face = NULL;
    data->face_index = face_index;
    data->face_type = face_type;
    IDWriteFontFile_AddRef(file);
    IDWriteFactory_AddRef(factory);

    opentype_get_font_table(stream, face_type, face_index, MS_OS2_TAG, &tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(stream, face_type, face_index, MS_HEAD_TAG, &tt_head, &head_context, NULL, NULL);
    opentype_get_font_table(stream, face_type, face_index, MS_POST_TAG, &tt_post, &post_context, NULL, NULL);

    get_font_properties(tt_os2, tt_head, tt_post, &data->metrics, &data->stretch, &data->weight, &data->style);

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(stream, os2_context);
    if (tt_head)
        IDWriteFontFileStream_ReleaseFileFragment(stream, head_context);
    if (tt_post)
        IDWriteFontFileStream_ReleaseFileFragment(stream, post_context);
    IDWriteFontFileStream_Release(stream);

    return S_OK;
}

static HRESULT init_fontfamily_data(IDWriteLocalizedStrings *familyname, struct dwrite_fontfamily_data *data)
{
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

    return S_OK;
}

HRESULT create_font_collection(IDWriteFactory* factory, IDWriteFontFileEnumerator *enumerator, IDWriteFontCollection **ret)
{
    struct dwrite_fontcollection *collection;
    BOOL current = FALSE;
    HRESULT hr;

    *ret = NULL;

    collection = heap_alloc(sizeof(struct dwrite_fontcollection));
    if (!collection) return E_OUTOFMEMORY;

    hr = init_font_collection(collection);
    if (FAILED(hr)) {
        heap_free(collection);
        return hr;
    }

    *ret = &collection->IDWriteFontCollection_iface;

    TRACE("building font collection:\n");

    while (1) {
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
            BOOL exists;

            /* alloc and init new font data structure */
            font_data = heap_alloc_zero(sizeof(struct dwrite_font_data));
            init_font_data(factory, file, i, face_type, font_data);

            hr = get_filestream_from_file(file, &stream);
            if (FAILED(hr))
                return hr;

            /* get family name from font file */
            name_table = NULL;
            opentype_get_font_table(stream, face_type, i, MS_NAME_TAG, &name_table, &name_context, NULL, NULL);
            if (name_table)
                hr = opentype_get_font_strings_from_id(name_table, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &family_name);
            IDWriteFontFileStream_Release(stream);

            if (FAILED(hr) || !family_name) {
                WARN("unable to get family name from font\n");
                continue;
            }

            buffer[0] = 0;
            IDWriteLocalizedStrings_GetString(family_name, 0, buffer, sizeof(buffer)/sizeof(WCHAR));

            exists = FALSE;
            hr = collection_find_family(collection, buffer, &index, &exists);
            if (exists)
                hr = fontfamily_add_font(collection->family_data[index], font_data);
            else {
                struct dwrite_fontfamily_data *family_data;

                /* create and init new family */
                family_data = heap_alloc(sizeof(*family_data));
                init_fontfamily_data(family_name, family_data);

                /* add font to family, family - to collection */
                fontfamily_add_font(family_data, font_data);
                fontcollection_add_family(collection, family_data);
            }

            IDWriteLocalizedStrings_Release(family_name);
        }

        IDWriteFontFile_Release(file);
    };

    return S_OK;
}

static INT CALLBACK enum_font_families(const LOGFONTW *lf, const TEXTMETRICW *tm, DWORD type, LPARAM lParam)
{
    struct dwrite_fontcollection *collection = (struct dwrite_fontcollection*)lParam;
    return add_family_syscollection(collection, lf->lfFaceName) == S_OK;
}

HRESULT get_system_fontcollection(IDWriteFontCollection **collection)
{
    struct dwrite_fontcollection *This;
    LOGFONTW lf;
    HDC hdc;

    *collection = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontcollection));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontCollection_iface.lpVtbl = &fontcollectionvtbl;
    This->ref = 1;
    This->alloc = 50;
    This->count = 0;
    This->families = heap_alloc(This->alloc*sizeof(WCHAR*));
    if (!This->families)
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }
    This->family_count = 0;
    This->family_alloc = 2;
    This->family_data = heap_alloc(sizeof(*This->family_data)*2);
    if (!This->family_data)
    {
        heap_free(This->families);
        heap_free(This);
        return E_OUTOFMEMORY;
    }

    TRACE("building system font collection:\n");

    hdc = CreateCompatibleDC(0);
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    lf.lfFaceName[0] = 0;
    EnumFontFamiliesExW(hdc, &lf, enum_font_families, (LPARAM)This, 0);
    DeleteDC(hdc);

    *collection = &This->IDWriteFontCollection_iface;

    return S_OK;
}

static HRESULT create_fontfamily_from_data(struct dwrite_fontfamily_data *data, IDWriteFontCollection *collection, IDWriteFontFamily **family)
{
    struct dwrite_fontfamily *This;

    *family = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontfamily));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFamily_iface.lpVtbl = &fontfamilyvtbl;
    This->ref = 1;
    This->collection = collection;
    if (collection)
        IDWriteFontCollection_AddRef(collection);
    This->data = data;
    InterlockedIncrement(&This->data->ref);

    *family = &This->IDWriteFontFamily_iface;

    return S_OK;
}

static HRESULT create_fontfamily(IDWriteLocalizedStrings *familyname, IDWriteFontFamily **family)
{
    struct dwrite_fontfamily_data *data;
    HRESULT ret;

    data = heap_alloc(sizeof(struct dwrite_fontfamily_data));
    if (!data) return E_OUTOFMEMORY;

    data->ref = 0;
    data->font_count = 0;
    data->font_alloc = 2;
    data->fonts = heap_alloc(sizeof(*data->fonts) * 2);
    if (!data->fonts)
    {
        heap_free(data);
        return E_OUTOFMEMORY;
    }
    data->familyname = familyname;

    ret = create_fontfamily_from_data(data, NULL, family);
    if (FAILED(ret))
    {
        heap_free(data->fonts);
        heap_free(data);
    }

    return ret;
}

static HRESULT create_font_from_data(struct dwrite_font_data *data, IDWriteFontFamily *family, IDWriteFont **font)
{
    struct dwrite_font *This;
    *font = NULL;

    This = heap_alloc(sizeof(struct dwrite_font));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFont2_iface.lpVtbl = &dwritefontvtbl;
    This->ref = 1;
    This->family = family;
    if (family)
        IDWriteFontFamily_AddRef(family);
    This->is_system = FALSE;
    This->data = data;
    InterlockedIncrement(&This->data->ref);

    *font = (IDWriteFont*)&This->IDWriteFont2_iface;

    return S_OK;
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

    FIXME("(%p)->(%p, %p, %p, %p): Stub\n", This, isSupportedFontType, fontFileType, fontFaceType, numberOfFaces);

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

HRESULT create_fontface(DWRITE_FONT_FACE_TYPE facetype, UINT32 files_number, IDWriteFontFile* const* font_files, UINT32 index,
    DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace2 **ret)
{
    struct dwrite_fontface *fontface;
    HRESULT hr = S_OK;
    int i;

    fontface = heap_alloc(sizeof(struct dwrite_fontface));
    if (!fontface)
        return E_OUTOFMEMORY;

    fontface->files = heap_alloc(sizeof(*fontface->files) * files_number);
    if (!fontface->files) {
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

    /* Verify font file streams */
    for (i = 0; i < fontface->file_count && SUCCEEDED(hr); i++)
    {
        IDWriteFontFileStream *stream;
        hr = _dwritefontfile_GetFontFileStream(font_files[i], &stream);
        if (SUCCEEDED(hr))
            IDWriteFontFileStream_Release(stream);
    }

    if (FAILED(hr)) {
        heap_free(fontface->files);
        heap_free(fontface);
        return hr;
    }

    for (i = 0; i < fontface->file_count; i++) {
        fontface->files[i] = font_files[i];
        IDWriteFontFile_AddRef(font_files[i]);
    }

    fontface->index = index;
    fontface->simulations = simulations;
    fontface->is_system = FALSE;

    *ret = &fontface->IDWriteFontFace2_iface;
    return S_OK;
}

/* IDWriteLocalFontFileLoader and its required IDWriteFontFileStream */

struct dwrite_localfontfilestream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG ref;

    HANDLE handle;
};

struct dwrite_localfontfileloader {
    IDWriteLocalFontFileLoader IDWriteLocalFontFileLoader_iface;
    LONG ref;
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

static ULONG WINAPI localfontfilestream_Release(IDWriteFontFileStream *iface)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        if (This->handle != INVALID_HANDLE_VALUE)
            CloseHandle(This->handle);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI localfontfilestream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start, UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    LARGE_INTEGER distance;
    DWORD bytes = fragment_size;
    DWORD read;

    TRACE("(%p)->(%p, %s, %s, %p)\n",This, fragment_start,
          wine_dbgstr_longlong(offset), wine_dbgstr_longlong(fragment_size), fragment_context);

    *fragment_context = NULL;
    distance.QuadPart = offset;
    if (!SetFilePointerEx(This->handle, distance, NULL, FILE_BEGIN))
        return E_FAIL;
    *fragment_start = *fragment_context = heap_alloc(bytes);
    if (!*fragment_context)
        return E_FAIL;
    if (!ReadFile(This->handle, *fragment_context, bytes, &read, NULL))
    {
        heap_free(*fragment_context);
        return E_FAIL;
    }

    return S_OK;
}

static void WINAPI localfontfilestream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    TRACE("(%p)->(%p)\n", This, fragment_context);
    heap_free(fragment_context);
}

static HRESULT WINAPI localfontfilestream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    LARGE_INTEGER li;
    TRACE("(%p)->(%p)\n",This, size);
    GetFileSizeEx(This->handle, &li);
    *size = li.QuadPart;
    return S_OK;
}

static HRESULT WINAPI localfontfilestream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    struct dwrite_localfontfilestream *This = impl_from_IDWriteFontFileStream(iface);
    FIXME("(%p)->(%p): stub\n",This, last_writetime);
    *last_writetime = 0;
    return E_NOTIMPL;
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

static HRESULT create_localfontfilestream(HANDLE handle, IDWriteFontFileStream** iface)
{
    struct dwrite_localfontfilestream *This = heap_alloc(sizeof(struct dwrite_localfontfilestream));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->handle = handle;
    This->IDWriteFontFileStream_iface.lpVtbl = &localfontfilestreamvtbl;

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

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI localfontfileloader_CreateStreamFromKey(IDWriteLocalFontFileLoader *iface, const void *fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream **fontFileStream)
{
    HANDLE handle;
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    const WCHAR *name = (const WCHAR*)fontFileReferenceKey;

    TRACE("(%p)->(%p, %i, %p)\n",This, fontFileReferenceKey, fontFileReferenceKeySize, fontFileStream);

    TRACE("name: %s\n",debugstr_w(name));
    handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return E_FAIL;

    return create_localfontfilestream(handle, fontFileStream);
}

static HRESULT WINAPI localfontfileloader_GetFilePathLengthFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, UINT32 *length)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    TRACE("(%p)->(%p, %i, %p)\n",This, key, key_size, length);
    *length = key_size;
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetFilePathFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, WCHAR *path, UINT32 length)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    TRACE("(%p)->(%p, %i, %p, %i)\n",This, key, key_size, path, length);
    if (length < key_size)
        return E_INVALIDARG;
    lstrcpynW((WCHAR*)key, path, key_size);
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetLastWriteTimeFromKey(IDWriteLocalFontFileLoader *iface, void const *key, UINT32 key_size, FILETIME *writetime)
{
    struct dwrite_localfontfileloader *This = impl_from_IDWriteLocalFontFileLoader(iface);
    FIXME("(%p)->(%p, %i, %p):stub\n",This, key, key_size, writetime);
    return E_NOTIMPL;
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

    This->ref = 1;
    This->IDWriteLocalFontFileLoader_iface.lpVtbl = &localfontfileloadervtbl;

    *iface = &This->IDWriteLocalFontFileLoader_iface;
    return S_OK;
}
