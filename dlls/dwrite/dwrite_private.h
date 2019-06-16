/*
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

#include "dwrite_3.h"
#include "d2d1.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"
#include "wine/unicode.h"

static const DWRITE_MATRIX identity =
{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
};

static inline LPWSTR heap_strdupW(const WCHAR *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strdupnW(const WCHAR *str, UINT32 len)
{
    WCHAR *ret = NULL;

    if (len)
    {
        ret = heap_alloc((len+1)*sizeof(WCHAR));
        if(ret)
        {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
    }

    return ret;
}

static inline const char *debugstr_range(const DWRITE_TEXT_RANGE *range)
{
    return wine_dbg_sprintf("%u:%u", range->startPosition, range->length);
}

static inline const char *debugstr_matrix(const DWRITE_MATRIX *m)
{
    if (!m) return "(null)";
    return wine_dbg_sprintf("{%.2f,%.2f,%.2f,%.2f,%.2f,%.2f}", m->m11, m->m12, m->m21, m->m22,
        m->dx, m->dy);
}

static inline BOOL dwrite_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = heap_realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static inline const char *debugstr_tag(DWORD tag)
{
    return debugstr_an((char *)&tag, 4);
}

const char *debugstr_sa_script(UINT16) DECLSPEC_HIDDEN;

static inline unsigned short get_table_entry(const unsigned short *table, WCHAR ch)
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

static inline FLOAT get_scaled_advance_width(INT32 advance, FLOAT emSize, const DWRITE_FONT_METRICS *metrics)
{
    return (FLOAT)advance * emSize / (FLOAT)metrics->designUnitsPerEm;
}

static inline BOOL is_simulation_valid(DWRITE_FONT_SIMULATIONS simulations)
{
    return (simulations & ~(DWRITE_FONT_SIMULATIONS_NONE | DWRITE_FONT_SIMULATIONS_BOLD |
        DWRITE_FONT_SIMULATIONS_OBLIQUE)) == 0;
}

struct textlayout_desc
{
    IDWriteFactory5 *factory;
    const WCHAR *string;
    UINT32 length;
    IDWriteTextFormat *format;
    FLOAT max_width;
    FLOAT max_height;
    BOOL is_gdi_compatible;
    /* fields below are only meaningful for gdi-compatible layout */
    FLOAT ppdip;
    const DWRITE_MATRIX *transform;
    BOOL use_gdi_natural;
};

struct glyphrunanalysis_desc
{
    const DWRITE_GLYPH_RUN *run;
    const DWRITE_MATRIX *transform;
    DWRITE_RENDERING_MODE1 rendering_mode;
    DWRITE_MEASURING_MODE measuring_mode;
    DWRITE_GRID_FIT_MODE gridfit_mode;
    DWRITE_TEXT_ANTIALIAS_MODE aa_mode;
    D2D_POINT_2F origin;
};

struct fontface_desc
{
    IDWriteFactory5 *factory;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFile * const *files;
    IDWriteFontFileStream *stream;
    UINT32 files_number;
    UINT32 index;
    DWRITE_FONT_SIMULATIONS simulations;
    struct dwrite_font_data *font_data; /* could be NULL when face is created directly with IDWriteFactory::CreateFontFace() */
};

struct dwrite_fonttable
{
    const BYTE *data;
    void *context;
    UINT32 size;
    BOOL exists;
};

struct fontfacecached
{
    struct list entry;
    IDWriteFontFace4 *fontface;
};

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

struct dwrite_fontface
{
    IDWriteFontFace4 IDWriteFontFace4_iface;
    LONG ref;

    IDWriteFontFileStream *stream;
    IDWriteFontFile **files;
    UINT32 file_count;
    UINT32 index;

    IDWriteFactory5 *factory;
    struct fontfacecached *cached;

    USHORT simulations;
    DWRITE_FONT_FACE_TYPE type;
    DWRITE_FONT_METRICS1 metrics;
    DWRITE_CARET_METRICS caret;
    struct
    {
        unsigned int ascent;
        unsigned int descent;
    } typo_metrics;
    INT charmap;
    UINT16 flags;

    struct dwrite_fonttable cmap;
    struct dwrite_fonttable vdmx;
    struct dwrite_fonttable gasp;
    struct dwrite_fonttable cpal;
    struct dwrite_fonttable colr;
    DWRITE_GLYPH_METRICS *glyphs[GLYPH_MAX/GLYPH_BLOCK_SIZE];

    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_PANOSE panose;
    FONTSIGNATURE fontsig;
    UINT32 glyph_image_formats;

    struct scriptshaping_cache *shaping_cache;

    LOGFONTW lf;
};

extern HRESULT create_numbersubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD,const WCHAR *locale,BOOL,IDWriteNumberSubstitution**) DECLSPEC_HIDDEN;
extern HRESULT create_textformat(const WCHAR*,IDWriteFontCollection*,DWRITE_FONT_WEIGHT,DWRITE_FONT_STYLE,DWRITE_FONT_STRETCH,
                                 FLOAT,const WCHAR*,IDWriteTextFormat**) DECLSPEC_HIDDEN;
extern HRESULT create_textlayout(const struct textlayout_desc*,IDWriteTextLayout**) DECLSPEC_HIDDEN;
extern HRESULT create_trimmingsign(IDWriteFactory5*,IDWriteTextFormat*,IDWriteInlineObject**) DECLSPEC_HIDDEN;
extern HRESULT create_typography(IDWriteTypography**) DECLSPEC_HIDDEN;
extern HRESULT create_localizedstrings(IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT add_localizedstring(IDWriteLocalizedStrings*,const WCHAR*,const WCHAR*) DECLSPEC_HIDDEN;
extern HRESULT clone_localizedstring(IDWriteLocalizedStrings *iface, IDWriteLocalizedStrings **strings) DECLSPEC_HIDDEN;
extern void    set_en_localizedstring(IDWriteLocalizedStrings*,const WCHAR*) DECLSPEC_HIDDEN;
extern HRESULT get_system_fontcollection(IDWriteFactory5*,IDWriteFontCollection1**) DECLSPEC_HIDDEN;
extern HRESULT get_eudc_fontcollection(IDWriteFactory5*,IDWriteFontCollection1**) DECLSPEC_HIDDEN;
extern IDWriteTextAnalyzer *get_text_analyzer(void) DECLSPEC_HIDDEN;
extern HRESULT create_font_file(IDWriteFontFileLoader *loader, const void *reference_key, UINT32 key_size, IDWriteFontFile **font_file) DECLSPEC_HIDDEN;
extern void    init_local_fontfile_loader(void) DECLSPEC_HIDDEN;
extern IDWriteFontFileLoader *get_local_fontfile_loader(void) DECLSPEC_HIDDEN;
extern HRESULT create_fontface(const struct fontface_desc*,struct list*,IDWriteFontFace4**) DECLSPEC_HIDDEN;
extern HRESULT create_font_collection(IDWriteFactory5*,IDWriteFontFileEnumerator*,BOOL,IDWriteFontCollection1**) DECLSPEC_HIDDEN;
extern HRESULT create_glyphrunanalysis(const struct glyphrunanalysis_desc*,IDWriteGlyphRunAnalysis**) DECLSPEC_HIDDEN;
extern BOOL    is_system_collection(IDWriteFontCollection*) DECLSPEC_HIDDEN;
extern HRESULT get_local_refkey(const WCHAR*,const FILETIME*,void**,UINT32*) DECLSPEC_HIDDEN;
extern HRESULT get_filestream_from_file(IDWriteFontFile*,IDWriteFontFileStream**) DECLSPEC_HIDDEN;
extern BOOL    is_face_type_supported(DWRITE_FONT_FACE_TYPE) DECLSPEC_HIDDEN;
extern HRESULT get_family_names_from_stream(IDWriteFontFileStream*,UINT32,DWRITE_FONT_FACE_TYPE,IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT create_colorglyphenum(FLOAT,FLOAT,const DWRITE_GLYPH_RUN*,const DWRITE_GLYPH_RUN_DESCRIPTION*,DWRITE_MEASURING_MODE,
    const DWRITE_MATRIX*,UINT32,IDWriteColorGlyphRunEnumerator**) DECLSPEC_HIDDEN;
extern BOOL lb_is_newline_char(WCHAR) DECLSPEC_HIDDEN;
extern HRESULT create_system_fontfallback(IDWriteFactory5*,IDWriteFontFallback**) DECLSPEC_HIDDEN;
extern void    release_system_fontfallback(IDWriteFontFallback*) DECLSPEC_HIDDEN;
extern HRESULT create_fontfallback_builder(IDWriteFactory5*,IDWriteFontFallbackBuilder**) DECLSPEC_HIDDEN;
extern HRESULT create_matching_font(IDWriteFontCollection*,const WCHAR*,DWRITE_FONT_WEIGHT,DWRITE_FONT_STYLE,DWRITE_FONT_STRETCH,
    IDWriteFont**) DECLSPEC_HIDDEN;
extern HRESULT create_fontfacereference(IDWriteFactory5*,IDWriteFontFile*,UINT32,DWRITE_FONT_SIMULATIONS,
    IDWriteFontFaceReference**) DECLSPEC_HIDDEN;
extern HRESULT factory_get_cached_fontface(IDWriteFactory5*,IDWriteFontFile*const*,UINT32,DWRITE_FONT_SIMULATIONS,
        struct list**,REFIID,void**) DECLSPEC_HIDDEN;
extern void factory_detach_fontcollection(IDWriteFactory5*,IDWriteFontCollection1*) DECLSPEC_HIDDEN;
extern void factory_detach_gdiinterop(IDWriteFactory5*,IDWriteGdiInterop1*) DECLSPEC_HIDDEN;
extern struct fontfacecached *factory_cache_fontface(IDWriteFactory5*,struct list*,IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern void    get_logfont_from_font(IDWriteFont*,LOGFONTW*) DECLSPEC_HIDDEN;
extern void    get_logfont_from_fontface(IDWriteFontFace*,LOGFONTW*) DECLSPEC_HIDDEN;
extern HRESULT get_fontsig_from_font(IDWriteFont*,FONTSIGNATURE*) DECLSPEC_HIDDEN;
extern HRESULT get_fontsig_from_fontface(IDWriteFontFace*,FONTSIGNATURE*) DECLSPEC_HIDDEN;
extern HRESULT create_gdiinterop(IDWriteFactory5*,IDWriteGdiInterop1**) DECLSPEC_HIDDEN;
extern void fontface_detach_from_cache(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern void factory_lock(IDWriteFactory5*) DECLSPEC_HIDDEN;
extern void factory_unlock(IDWriteFactory5*) DECLSPEC_HIDDEN;
extern HRESULT create_inmemory_fileloader(IDWriteFontFileLoader**) DECLSPEC_HIDDEN;

struct dwrite_fontface;

extern float fontface_get_scaled_design_advance(struct dwrite_fontface *fontface, DWRITE_MEASURING_MODE measuring_mode,
        float emsize, float ppdip, const DWRITE_MATRIX *transform, UINT16 glyph, BOOL is_sideways) DECLSPEC_HIDDEN;
extern struct dwrite_fontface *unsafe_impl_from_IDWriteFontFace(IDWriteFontFace *iface) DECLSPEC_HIDDEN;

/* Opentype font table functions */
struct dwrite_font_props {
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_PANOSE panose;
    FONTSIGNATURE fontsig;
    LOGFONTW lf;
};

struct file_stream_desc {
    IDWriteFontFileStream *stream;
    DWRITE_FONT_FACE_TYPE face_type;
    UINT32 face_index;
};

extern const void* get_fontface_table(IDWriteFontFace4 *fontface, UINT32 tag,
        struct dwrite_fonttable *table) DECLSPEC_HIDDEN;

extern HRESULT opentype_analyze_font(IDWriteFontFileStream*,BOOL*,DWRITE_FONT_FILE_TYPE*,DWRITE_FONT_FACE_TYPE*,UINT32*) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_table(struct file_stream_desc*,UINT32,const void**,void**,UINT32*,BOOL*) DECLSPEC_HIDDEN;
extern HRESULT opentype_cmap_get_unicode_ranges(const struct dwrite_fonttable *table, unsigned int max_count,
        DWRITE_UNICODE_RANGE *ranges, unsigned int *count) DECLSPEC_HIDDEN;
extern void opentype_get_font_properties(struct file_stream_desc*,struct dwrite_font_props*) DECLSPEC_HIDDEN;
extern void opentype_get_font_metrics(struct file_stream_desc*,DWRITE_FONT_METRICS1*,DWRITE_CARET_METRICS*) DECLSPEC_HIDDEN;
extern void opentype_get_font_typo_metrics(struct file_stream_desc *stream_desc, unsigned int *ascent,
        unsigned int *descent) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_info_strings(const void*,DWRITE_INFORMATIONAL_STRING_ID,IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_familyname(struct file_stream_desc*,IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_facename(struct file_stream_desc*,WCHAR*,IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_typographic_features(IDWriteFontFace*,UINT32,UINT32,UINT32,UINT32*,DWRITE_FONT_FEATURE_TAG*) DECLSPEC_HIDDEN;
extern BOOL opentype_get_vdmx_size(const struct dwrite_fonttable *table, INT ppem, UINT16 *ascent,
        UINT16 *descent) DECLSPEC_HIDDEN;
extern unsigned int opentype_get_cpal_palettecount(const struct dwrite_fonttable *table) DECLSPEC_HIDDEN;
extern unsigned int opentype_get_cpal_paletteentrycount(const struct dwrite_fonttable *table) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_cpal_entries(const struct dwrite_fonttable *table, unsigned int palette,
        unsigned int first_entry_index, unsigned int entry_count, DWRITE_COLOR_F *entries) DECLSPEC_HIDDEN;
extern BOOL opentype_has_vertical_variants(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern UINT32 opentype_get_glyph_image_formats(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern DWRITE_CONTAINER_TYPE opentype_analyze_container_type(void const *, UINT32) DECLSPEC_HIDDEN;

struct dwrite_colorglyph {
    USHORT layer; /* [0, num_layers) index indicating current layer */
    /* base glyph record data, set once on initialization */
    USHORT first_layer;
    USHORT num_layers;
    /* current layer record data, updated every time glyph is switched to next layer */
    UINT16 glyph;
    UINT16 palette_index;
};

extern HRESULT opentype_get_colr_glyph(const struct dwrite_fonttable *table, UINT16 glyph,
        struct dwrite_colorglyph *color_glyph) DECLSPEC_HIDDEN;
extern void opentype_colr_next_glyph(const struct dwrite_fonttable *table,
        struct dwrite_colorglyph *color_glyph) DECLSPEC_HIDDEN;

enum gasp_flags {
    GASP_GRIDFIT             = 0x0001,
    GASP_DOGRAY              = 0x0002,
    GASP_SYMMETRIC_GRIDFIT   = 0x0004,
    GASP_SYMMETRIC_SMOOTHING = 0x0008,
};

extern unsigned int opentype_get_gasp_flags(const struct dwrite_fonttable *gasp, float emsize) DECLSPEC_HIDDEN;

/* BiDi helpers */
extern HRESULT bidi_computelevels(const WCHAR*,UINT32,UINT8,UINT8*,UINT8*) DECLSPEC_HIDDEN;
extern WCHAR bidi_get_mirrored_char(WCHAR) DECLSPEC_HIDDEN;

/* FreeType integration */
struct dwrite_glyphbitmap
{
    IDWriteFontFace4 *fontface;
    DWORD simulations;
    float emsize;
    BOOL nohint;
    BOOL aliased;
    UINT16 glyph;
    INT pitch;
    RECT bbox;
    BYTE *buf;
    DWRITE_MATRIX *m;
};

extern BOOL init_freetype(void) DECLSPEC_HIDDEN;
extern void release_freetype(void) DECLSPEC_HIDDEN;

extern HRESULT freetype_get_design_glyph_metrics(struct dwrite_fontface *fontface, UINT16 glyph,
        DWRITE_GLYPH_METRICS *metrics) DECLSPEC_HIDDEN;
extern void freetype_notify_cacheremove(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern BOOL freetype_is_monospaced(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern HRESULT freetype_get_glyphrun_outline(IDWriteFontFace4 *fontface, float emsize, UINT16 const *glyphs,
        float const *advances, DWRITE_GLYPH_OFFSET const *offsets, unsigned int count, BOOL is_rtl,
        IDWriteGeometrySink *sink) DECLSPEC_HIDDEN;
extern UINT16 freetype_get_glyphcount(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern void freetype_get_glyphs(IDWriteFontFace4*,INT,UINT32 const*,UINT32,UINT16*) DECLSPEC_HIDDEN;
extern BOOL freetype_has_kerning_pairs(IDWriteFontFace4*) DECLSPEC_HIDDEN;
extern INT32 freetype_get_kerning_pair_adjustment(IDWriteFontFace4*,UINT16,UINT16) DECLSPEC_HIDDEN;
extern void freetype_get_glyph_bbox(struct dwrite_glyphbitmap *bitmap_desc) DECLSPEC_HIDDEN;
extern BOOL freetype_get_glyph_bitmap(struct dwrite_glyphbitmap*) DECLSPEC_HIDDEN;
extern INT freetype_get_charmap_index(IDWriteFontFace4*,BOOL*) DECLSPEC_HIDDEN;
extern INT32 freetype_get_glyph_advance(IDWriteFontFace4*,FLOAT,UINT16,DWRITE_MEASURING_MODE,BOOL*) DECLSPEC_HIDDEN;
extern void freetype_get_design_glyph_bbox(IDWriteFontFace4*,UINT16,UINT16,RECT*) DECLSPEC_HIDDEN;

/* Glyph shaping */
enum SCRIPT_JUSTIFY
{
    SCRIPT_JUSTIFY_NONE,
    SCRIPT_JUSTIFY_ARABIC_BLANK,
    SCRIPT_JUSTIFY_CHARACTER,
    SCRIPT_JUSTIFY_RESERVED1,
    SCRIPT_JUSTIFY_BLANK,
    SCRIPT_JUSTIFY_RESERVED2,
    SCRIPT_JUSTIFY_RESERVED3,
    SCRIPT_JUSTIFY_ARABIC_NORMAL,
    SCRIPT_JUSTIFY_ARABIC_KASHIDA,
    SCRIPT_JUSTIFY_ARABIC_ALEF,
    SCRIPT_JUSTIFY_ARABIC_HA,
    SCRIPT_JUSTIFY_ARABIC_RA,
    SCRIPT_JUSTIFY_ARABIC_BA,
    SCRIPT_JUSTIFY_ARABIC_BARA,
    SCRIPT_JUSTIFY_ARABIC_SEEN,
    SCRIPT_JUSTIFY_ARABIC_SEEN_M
};

struct scriptshaping_cache
{
    const struct shaping_font_ops *font;
    void *context;
    UINT16 upem;

    struct
    {
        struct dwrite_fonttable table;
        unsigned int script_list;
        unsigned int feature_list;
        unsigned int lookup_list;
    } gpos;

    struct
    {
        struct dwrite_fonttable table;
        unsigned int classdef;
    } gdef;
};

struct scriptshaping_context
{
    struct scriptshaping_cache *cache;
    UINT32 language_tag;

    const WCHAR *text;
    unsigned int length;
    BOOL is_rtl;

    union
    {
        struct
        {
            const UINT16 *glyphs;
            const DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props;
        } pos;
    } u;

    unsigned int glyph_count;
    float emsize;
    DWRITE_MEASURING_MODE measuring_mode;
    float *advances;
    DWRITE_GLYPH_OFFSET *offsets;
};

struct shaping_font_ops
{
    void (*grab_font_table)(void *context, UINT32 table, const BYTE **data, UINT32 *size, void **data_context);
    void (*release_font_table)(void *context, void *data_context);
    UINT16 (*get_font_upem)(void *context);
};

extern struct scriptshaping_cache *create_scriptshaping_cache(void *context,
        const struct shaping_font_ops *font_ops) DECLSPEC_HIDDEN;
extern void release_scriptshaping_cache(struct scriptshaping_cache*) DECLSPEC_HIDDEN;
extern struct scriptshaping_cache *fontface_get_shaping_cache(struct dwrite_fontface *fontface) DECLSPEC_HIDDEN;

struct shaping_features
{
    const DWORD *tags;
    unsigned int count;
};

extern void opentype_layout_scriptshaping_cache_init(struct scriptshaping_cache *cache) DECLSPEC_HIDDEN;
extern DWORD opentype_layout_find_script(const struct scriptshaping_cache *cache, DWORD kind, DWORD tag,
        unsigned int *script_index) DECLSPEC_HIDDEN;
extern DWORD opentype_layout_find_language(const struct scriptshaping_cache *cache, DWORD kind, DWORD tag,
        unsigned int script_index, unsigned int *language_index) DECLSPEC_HIDDEN;
extern void opentype_layout_apply_gpos_features(struct scriptshaping_context *context, unsigned int script_index,
        unsigned int language_index, const struct shaping_features *features) DECLSPEC_HIDDEN;

struct scriptshaping_ops
{
    HRESULT (*contextual_shaping)(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices, UINT32* actual_glyph_count);
    HRESULT (*set_text_glyphs_props)(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props);
    const struct shaping_features *gpos_features;
};

extern const struct scriptshaping_ops default_shaping_ops DECLSPEC_HIDDEN;
extern const struct scriptshaping_ops latn_shaping_ops DECLSPEC_HIDDEN;

extern HRESULT shape_get_positions(struct scriptshaping_context *context, const DWORD *scripts,
        const struct shaping_features *features) DECLSPEC_HIDDEN;
